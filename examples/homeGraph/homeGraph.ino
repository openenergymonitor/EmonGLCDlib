/// @dir homeGraph
/// Display latest home power consumption info, including a historical graph.
/// @see http://jeelabs.org/2012/10/25/who-needs-a-server/
// 2012-10-21 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <GLCD_ST7565.h>
#include <JeeLib.h>
#include "utility/font_clR6x6.h"
#include "utility/font_8x13B.h"
#include <avr/eeprom.h>

#define GROUP   5   // only listen to data in netgroup 5
#define SEND_ID 9   // node 9 is the homePower.ino node

#define NUMPINS 3   // number of counters in each packet
#define NUMHIST 20  // number of graph points, i.e. 5 hours

#define EE_BASE 100 // base address in EEPROM to save graph data

enum { COOKING, SOLAR, HOME }; // order of incoming payload items

struct PayloadItem { word count; word tdiff; };

GLCD_ST7565 glcd;
MilliTimer receiveTimer, graphTimer;
byte minutes; // graph data is shifted left every 15 minutes

// additional state needed to track incoming kwh totals
bool firstTime = true;      // true after restart
word lastSolar, lastTotal;  // last counts received
word solarHist [NUMHIST];   // 15-min bins of solar KWh values
word totalHist [NUMHIST];   // 15-min bins of total KWh values

ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// reload graph data from EEPROM on power-up, unless invalid
static void loadGraphData () {
  if (eeprom_read_word((word*) EE_BASE) < 10000) {
    char* ptr = (char*) EE_BASE;
    eeprom_read_block(solarHist, ptr, sizeof solarHist);
    ptr += sizeof solarHist;
    eeprom_read_block(totalHist, ptr, sizeof totalHist);
  }
}

// move graph left every 15 minutes, and also save it to EEPROM
static void scrollAndSaveGraphData () {
  // shift the data, clear last item
  for (byte i = 0; i < NUMHIST-1; ++i) {
    solarHist[i] = solarHist[i+1];
    totalHist[i] = totalHist[i+1];
  }
  solarHist[NUMHIST-1] = totalHist[NUMHIST-1] = 0;
  // save data to EEPROM to (sort of) recover from power-down/reset
  char* ptr = (char*) EE_BASE;
  eeprom_write_block(solarHist, ptr, sizeof solarHist);
  ptr += sizeof solarHist;
  eeprom_write_block(totalHist, ptr, sizeof totalHist);
}

// scale and display the graph on leftmost half of the display
static void showGraph () {
  // determine scale limits
  word maxSolar = 0, maxTotal = 0;
  for (byte i = 0; i < NUMHIST; ++i) {
    if (solarHist[i] > maxSolar) maxSolar = solarHist[i];
    if (totalHist[i] > maxTotal) maxTotal = totalHist[i];
  }
  // scale the graph
  word halfWattPerTick = (maxSolar + maxTotal + 55) / 56;
  if (halfWattPerTick < 1)
    halfWattPerTick = 1;
  word baseline = 2 + maxSolar / halfWattPerTick;
  glcd.drawLine(0, baseline, 80, baseline, 1);
  // draw the hourly dots
  for (byte i = 0; i < NUMHIST/4; ++i)
    glcd.setPixel(16 * i + 2, 0, 1);
  // draw the elements
  for (byte i = 0; i < NUMHIST; ++i) {
    byte sHeight = solarHist[i] / halfWattPerTick;
    byte tHeight = totalHist[i] / halfWattPerTick;
    byte x = 4 * i + 1;
    glcd.fillRect(x, baseline - sHeight + 1, 3, sHeight, 1);
    glcd.drawRect(x, baseline, 3, tHeight, 1);
  }  
}

// show a single power value on the graphics display
static void showPower (word value, byte ypos) {
  char buf[6];
  if (value <= 100 || value >= 65000)
    strcpy(buf, "    -");
  else {
    long ms = value;
    if (value > 60000)
      ms = 1000L * (value - 60000);
    sprintf(buf, "%5lu", 1800000L / ms);
  }
  glcd.drawString(85,  ypos, buf);
}

// calculate totals for summary line and return it as a nice string
static const char* summaryLine () {
  // this won't overflow up to an average consumption rate of 6 KW
  // 6 KW x 5 hr x 2 pulses/Watt still fits in 16-bit unsigned int
  word sSum = 0, tSum = 0;
  for (byte i = 0; i < NUMHIST; ++i) {
    sSum += solarHist[i];
    tSum += totalHist[i];
  }
  // correction because counts are in units of 0.5 W
  sSum /= 2;
  tSum /= 2;
  // avoid floating point, but still display with 1 or 2 decimals
  static char buf [20];
  if (sSum <= 9999)
    sprintf(buf, "kWh +%d.%02d", sSum / 1000, (sSum / 10) % 100);
  else
    sprintf(buf, "KWh +%02d.%d", sSum / 1000, (sSum / 100) % 10);
  if (tSum <= 9999)
    sprintf(buf + 9, " -%d.%02d", tSum / 1000, (tSum / 10) % 100);
  else
    sprintf(buf + 9, " -%02d.%d", tSum / 1000, (tSum / 100) % 10);
  return buf;
}

// process the incoming counts, where one count is 0.5 Wh
static void processCounts (const struct PayloadItem* payload) {
  // keep track of the change for incoming counts
  word prevSolar = lastSolar, prevTotal = lastTotal;
  lastSolar = payload[SOLAR].count;
  lastTotal = payload[HOME].count + payload[COOKING].count;
  // careful with first data received after a restart
  if (firstTime) {
    prevSolar = lastSolar;
    prevTotal = lastTotal;
    firstTime = false;
  }
  // ignore excessive diffs, let's assume the sender has been reset
  word solarDiff = lastSolar - prevSolar;
  word totalDiff = lastTotal - prevTotal;
  if (solarDiff > 1000 || totalDiff > 1000)
    return; // i.e. reject if more than 1000 pulses were added
  // track the accumulated half-Wh values for graphing
  solarHist[NUMHIST-1] += solarDiff;
  totalHist[NUMHIST-1] += totalDiff;
}

// redraw entire display with the latest info
static void showPowerInfo (const struct PayloadItem* payload) {
  glcd.clear();
  glcd.setFont(font_clR6x6);
  glcd.drawString_P(85,  0, PSTR("Solar W"));
  glcd.drawString_P(85, 22, PSTR(" Home W"));
  glcd.drawString_P(85, 44, PSTR("Cooking"));
  glcd.drawString(0, 58, summaryLine());
  glcd.setFont(font_8x13B);
  showPower(payload[SOLAR].tdiff, 8);
  showPower(payload[HOME].tdiff, 30);
  showPower(payload[COOKING].tdiff, 52);
  showGraph();
  glcd.refresh();
}

// go to sleep, wakeup again just before the next packet
static void snoozeJustEnough (bool timingWasGood) {
  const word recvWindow = 150;
  word recvOffTime = 3000;
  if (!timingWasGood)
    recvOffTime -= recvWindow;

  rf12_sleep(RF12_SLEEP);
  // the backlight offers an easy way to show when the radio is on
  //glcd.backLight(0);
  Sleepy::loseSomeTime(recvOffTime);
  //glcd.backLight(10);
  rf12_sleep(RF12_WAKEUP);
  
  if (timingWasGood)
    receiveTimer.set(recvWindow);
}

void setup () {
  glcd.begin();
  glcd.backLight(0);
  glcd.refresh();
  rf12_initialize(1, RF12_868MHZ, GROUP);
  loadGraphData();
  // show some info on startup, before the first packet comes in
  showPowerInfo((const struct PayloadItem*) rf12_data);
}

void loop () {
  if (rf12_recvDone() && rf12_crc == 0 && rf12_hdr == SEND_ID &&
                rf12_len >= NUMPINS * sizeof (struct PayloadItem)) {
    processCounts((const struct PayloadItem*) rf12_data);
    showPowerInfo((const struct PayloadItem*) rf12_data);
    snoozeJustEnough(true);
  } else if (receiveTimer.poll())
    snoozeJustEnough(false);

  if (graphTimer.poll(60000) && ++minutes >= 15) {
    scrollAndSaveGraphData();
    minutes = 0;
  }
}
