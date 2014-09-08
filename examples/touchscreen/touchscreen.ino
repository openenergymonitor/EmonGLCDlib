// Touch screen demonstration program.
// 2010-12-22 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <GLCD_ST7565.h>
#include <JeeLib.h>
#include "utility/font_clR6x8.h"

GLCD_ST7565 glcd;
Port xport (2), yport (3);
int xlow, xhigh, ylow, yhigh, xpos, ypos;

static int GetOnePos(Port& port1, Port& port2, word low, word high, byte range) {
    port1.mode(OUTPUT);
    port1.mode2(OUTPUT);
    port1.digiWrite2(LOW);
    port1.digiWrite(HIGH);
 
    port2.digiWrite(LOW);
    port2.digiWrite2(LOW);
    port2.mode(INPUT);
    port2.mode2(INPUT); 
    delay(2);
 
	word value = port2.anaRead();
    return value < low ? -1 : map(value, low, high, 0, range);
}

static void GetTouchRaw(int& x, int& y) {
    x = GetOnePos(xport, yport, 0, 1024, 1024);
    y = GetOnePos(yport, xport, 0, 1024, 1024);
}

static void WaitNoTouch() {
    glcd.refresh();
	while (GetOnePos(xport, yport, 0, 1024, 1024) >= 0)
		;
    glcd.clear();
}

void GetTouchPos(){
    xpos = GetOnePos(xport, yport, xlow, xhigh, LCDWIDTH-1);
    ypos = GetOnePos(yport, xport, ylow, yhigh, LCDHEIGHT-1);
}

void CalibrateTouch(){
    glcd.clear();  
    glcd.drawString(3,25,"Touch the two corner");
    glcd.drawString(3,33,"points when they are");
    glcd.drawString(36,41,"indicated");
    glcd.drawTriangle(2,2,8,2,2,8,1);
    glcd.drawLine(5,5,8,8,1);
    glcd.refresh();

    glcd.setPixel(0,0,1);
    glcd.refresh();
    do
        GetTouchRaw(xlow, ylow);
    while (!xlow);
    glcd.setPixel(0,0,0);
	WaitNoTouch();

    glcd.drawTriangle(LCDWIDTH-3,LCDHEIGHT-3,
                      LCDWIDTH-9,LCDHEIGHT-3,
                      LCDWIDTH-3,LCDHEIGHT-9,1);
    glcd.drawLine(LCDWIDTH-6,LCDHEIGHT-6,
                  LCDWIDTH-9,LCDHEIGHT-9,1);
    glcd.refresh();
  
    glcd.setPixel(LCDWIDTH-1,LCDHEIGHT-1,1);
    glcd.refresh();
    do
        GetTouchRaw(xhigh, yhigh);
    while (!xhigh);
    glcd.setPixel(LCDWIDTH-1,LCDHEIGHT-1,0);
	WaitNoTouch();

    glcd.drawString(25,20,"Thank you");
    glcd.drawString(3,28,"Press OK to continue");
    glcd.drawString(52,42,"OK");
    glcd.drawRect(50,40,15,11,1);
    glcd.refresh();  

	do
        GetTouchPos();
	while (xpos<50 || xpos>65 || ypos<40 || ypos>51);
    WaitNoTouch();  
}

void setup(){
    glcd.begin();
    glcd.backLight(255);
    glcd.setFont(font_clR6x8);
    CalibrateTouch();
}
 
void loop(){
    GetTouchPos();
    if (xpos >= 0) {
        glcd.setPixel(xpos,ypos,1);
        glcd.refresh();
    }
}
