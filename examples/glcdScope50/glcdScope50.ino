// Very simple 50 Hz DSO scope, using the Graphics Board as display device.
// 2011-09-26 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>
#include <GLCD_ST7565.h>

GLCD_ST7565 glcd;
word samples[128];
MilliTimer timer;
byte rescans;

void setup () {
  Serial.begin(57600);
  Serial.println("\n[glcdScope50]");  
  glcd.begin();
  glcd.backLight(255);
}

void loop () {
  // synchronize to the millis() clock
  if (timer.poll(60)) {
    if (++rescans >= 10) {
      rescans = 0;
      glcd.clear();
    }

    // sample quickly with interrupts disabled
    for (byte i = 0; i < 128; ++i) {
      samples[i] = analogRead(2);
      delayMicroseconds(100);
    }
    // Serial.println(timer.remaining());

    // auto-scale the measured value
    long sum = 0;
    word min = 1023;
    word max = 0;
    for (byte i = 0; i < 128; ++i) {
      sum += samples[i];
      if (samples[i] > max) max = samples[i];
      if (samples[i] < min) min = samples[i];
    }
    word avg = (sum + 64) / 128;
    if (min == max) { min = 0; max = 1023; }

    // display values at max resolution, centered around the avg
    for (byte i = 0; i < 128; ++i) {
      // byte v = map(samples[i], min, max, 0, 63);
      byte v = map(samples[i], avg - 32, avg + 31, 0, 63);
      glcd.setPixel(i, v, 1);
    }

    glcd.refresh();
  }
}
