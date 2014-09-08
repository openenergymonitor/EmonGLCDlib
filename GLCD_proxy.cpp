// GLCD_proxy Remote LCD library.
// 2011-01-04 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <avr/pgmspace.h>
#include <Arduino.h>
#include <util/delay.h>
#include <stdlib.h>
#include <JeeLib.h>
#include "GLCD_proxy.h"
#include "GLCD_proxymsgs.h"

static byte msg[66];

#if 0
void GLCD_proxy::drawBitmap(byte x, byte y, 
                        const byte *bitmap, byte w, byte h, byte color) {
// No bitmap support yet - perhaps send thin bands, each as one packet?
    for (byte j=0; j<h; j++)
        for (byte i=0; i<w; i++ )
            if (pgm_read_byte(bitmap + i + (j/8)*w) & _BV(j%8))
                setPixel(x+i, y+j, color);
}
#endif

void GLCD_proxy::backLight(byte level) {
    msg[0]=REMOTE_GLCD_BACKLIGHT;
    msg[1]=level;
    sendLCDMessage(2);
}

void GLCD_proxy::drawString(byte x, byte y, const char *c) {
    msg[0]=REMOTE_GLCD_DRAWSTRING;
    msg[1]=x;
    msg[2]=y;
    byte* p = msg + 3;
    do
        *p = *c++;
    while (*p++);
    sendLCDMessage(p - msg);
}

void GLCD_proxy::drawString_P(byte x, byte y, const char *c) {
    msg[0]=REMOTE_GLCD_DRAWSTRING;
    msg[1]=x;
    msg[2]=y;
    byte* p = msg + 3;
    do
        *p = pgm_read_byte(c++);
    while (*p++);
    sendLCDMessage(p - msg);
}

void  GLCD_proxy::drawChar(byte x, byte y, char c) {
    msg[0]=REMOTE_GLCD_DRAWCHAR;
    msg[1]=x;
    msg[2]=y;
    msg[3]=c;
    sendLCDMessage(4);
}

void GLCD_proxy::drawLine(byte x0, byte y0, byte x1, byte y1, byte color) {
    msg[0]=REMOTE_GLCD_DRAWLINE;
    msg[1]=x0;
    msg[2]=y0;
    msg[3]=x1;
    msg[4]=y1;
    msg[5]=color;
    sendLCDMessage(6);
}

void GLCD_proxy::drawTriangle(byte x0, byte y0, byte x1, byte y1, byte x2, byte y2, byte color) {
    msg[0]=REMOTE_GLCD_DRAWTRIANGLE;
    msg[1]=x0;
    msg[2]=y0;
    msg[3]=x1;
    msg[4]=y1;
    msg[5]=x2;
    msg[6]=y2;
    msg[7]=color; 
    sendLCDMessage(8);
}

void GLCD_proxy::fillTriangle(byte x0, byte y0, byte x1, byte y1, byte x2, byte y2, byte color) {
    msg[0]=REMOTE_GLCD_FILLTRIANGLE;
    msg[1]=x0;
    msg[2]=y0;
    msg[3]=x1;
    msg[4]=y1;
    msg[5]=x2;
    msg[6]=y2;
    msg[7]=color; 
    sendLCDMessage(8);
}

void GLCD_proxy::fillRect(byte x, byte y, byte w, byte h, byte color) {
    msg[0]=REMOTE_GLCD_FILLRECT;
    msg[1]=x;
    msg[2]=y;
    msg[3]=w;
    msg[4]=h;
    msg[5]=color; 
    sendLCDMessage(6);
}

void GLCD_proxy::drawRect(byte x, byte y, byte w, byte h, byte color) {
    msg[0]=REMOTE_GLCD_DRAWRECT;
    msg[1]=x;
    msg[2]=y;
    msg[3]=w;
    msg[4]=h;
    msg[5]=color; 
    sendLCDMessage(6);
}

void GLCD_proxy::drawCircle(byte x0, byte y0, byte r, byte color) {
    msg[0]=REMOTE_GLCD_DRAWCIRCLE;
    msg[1]=x0;
    msg[2]=y0;
    msg[3]=r;
    msg[4]=color; 
    sendLCDMessage(5);
}

void GLCD_proxy::fillCircle(byte x0, byte y0, byte r, byte color) {
    msg[0]=REMOTE_GLCD_FILLCIRCLE;
    msg[1]=x0;
    msg[2]=y0;
    msg[3]=r;
    msg[4]=color; 
    sendLCDMessage(5);
}

void GLCD_proxy::setPixel(byte x, byte y, byte color) {
    msg[0]=REMOTE_GLCD_SETPIXEL;
    msg[1]=x;
    msg[2]=y;
    msg[3]=color; 
    sendLCDMessage(4);
}

void GLCD_proxy::refresh() {
    msg[0]=REMOTE_GLCD_REFRESH;
    sendLCDMessage(1);  
}

void GLCD_proxy::setUpdateArea(byte x0,byte y0,byte x1,byte y1, byte allowReduction) {
    msg[0]=REMOTE_GLCD_SETUPDATEAREA;
    msg[1]=x0;
    msg[2]=y0;
    msg[3]=x1;
    msg[4]=y1;
    msg[5]=allowReduction;  
    sendLCDMessage(6);
}

void GLCD_proxy::updateDisplayArea(byte x0,byte y0,byte x1,byte y1, byte reset) {
    msg[0]=REMOTE_GLCD_UPDATEAREA;
    msg[1]=x0;
    msg[2]=y0;
    msg[3]=x1;
    msg[4]=y1;
    msg[5]=reset; 
    sendLCDMessage(6);
}

void GLCD_proxy::scroll(byte direction,byte y) {
    msg[0]=REMOTE_GLCD_SCROLL;
    msg[1]=direction;
    msg[2]=y;
    sendLCDMessage(3);
}

void GLCD_proxy::clear() {
    msg[0]=REMOTE_GLCD_CLEAR;
    sendLCDMessage(1);
}

void GLCD_proxy::sendLCDMessage(byte length) {
    for (;;) {
        // send packet out once we can
        while (!rf12_canSend())
            rf12_recvDone();
        rf12_sendStart(RF12_HDR_ACK, msg, length);
	rf12_sendWait(1);
        // wait up to 100 ms to get an ack back
        MilliTimer t;
        while (!t.poll(100)) {
            // got an empty packet intended for us
            if (rf12_recvDone() && rf12_crc == 0 && rf12_len == 0)
                return;
        }
        // didn't get an ack, send again
    }
}
