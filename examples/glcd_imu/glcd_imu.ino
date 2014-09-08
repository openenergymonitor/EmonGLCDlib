// Display Gravity Plug and Pressure Plug info on a Graphics Board.
// 2012-05-18 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <GLCD_ST7565.h>
#include <JeeLib.h>
#include <PortsBMP085.h>
#include "utility/font_clR6x8.h"

class GraphicsBoard : public GLCD_ST7565, public Print {
    byte x, y, dirty;
    MilliTimer refreshTimer;
    
    void newline() {
        x = 0;
        if (y >= 58)
            scroll(SCROLLUP, 6);
        else
            y += 6;
    }
    
public:
    GraphicsBoard () : x (0), y (0), dirty (0) {}
    
    void cursor (byte xa, byte ya) {
      x = xa;
      y = ya;
    } 
    
    void poll (byte rate =100) {
        if (refreshTimer.poll(rate) && dirty) {
            refresh();
            dirty = 0;
        }
    }
    
    virtual size_t write (byte c) {
        if (c == '\r')
            x = 0;
        else if (c == '\n')
            x = 127;
        else {
            if (x > 122)
                newline();
            drawChar(x, y, c);
            x += 6;
            dirty = 1;
        }
        return 1;
    }
};

GraphicsBoard glcd;
PortI2C port2 (2), port3 (3);
BMP085 pressure (port2);
GravityPlug gravity (port3);

int d = 4;
byte x = 0;

void setup () {
  glcd.begin();
  glcd.backLight(255);
  glcd.setFont(font_clR6x8);
  glcd.refresh();

  pressure.getCalibData();
  gravity.begin();
}

static void barGraph (byte pos, char tag, int value) {
  glcd.cursor(0, pos);
  glcd.print(tag);
  glcd.print(' ');
  glcd.print(value);
  
  int scaled = 32 + value / 8;
  if (scaled < 0) scaled = 0;
  if (scaled > 63) scaled = 63;
  
  glcd.drawRect(0, pos+9, 64, 8, 1);
  glcd.fillRect(1, pos+10, scaled, 6, 1);
}

void loop () {
  int temp;
  long pres;

  // measure and convert temperature and pressure
  pressure.measure(BMP085::TEMP);
  pressure.measure(BMP085::PRES);
  pressure.calculate(temp, pres);
  
  glcd.clear();
  
  glcd.cursor(77, 10);
  glcd.print("Temp (C)");
  glcd.cursor(92, 20);
  glcd.print(temp * 0.1);
  
  glcd.cursor(65, 40);
  glcd.print("Pres (hPa)");
  glcd.cursor(80, 50);
  glcd.print(pres * 0.01);
  
  
  // get the array with X, Y, and Z readings
  const int* p = gravity.getAxes();
  
  barGraph(0, 'X', p[0]);
  barGraph(20, 'Y', p[1]);
  barGraph(40, 'Z', p[2]);
  

  glcd.refresh();
  delay(250);
}
