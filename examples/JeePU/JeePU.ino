// JeePU Display driver host application.
// 2011-06-01 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <GLCD_proxymsgs.h>
#include <GLCD_ST7565.h>
#include <JeeLib.h>
#include "utility/font_clR6x8.h"

#define NODE_ID     31
#define NODE_GROUP  212
#define NODE_FREQ   RF12_868MHZ

GLCD_ST7565 glcd;
byte doubleBuf[66];

void setup () {
    // Serial.begin(57600);
    // Serial.println("\n[JeePU]");
    glcd.begin();
    glcd.backLight(255);
    glcd.setFont(font_clR6x8);
    glcd.drawString(30,8,"JeePU Host");
    glcd.drawString(16,24,"Group:");
    char t[4];
    itoa(NODE_GROUP,t,10);
    glcd.drawString(52,24,t);
  
    glcd.drawString(84,24,"ID:");
    itoa(NODE_ID,t,10);
    glcd.drawString(102,24,t);
    glcd.drawLine(30,16,89,16,1);
    glcd.refresh(); 
    rf12_initialize(NODE_ID, NODE_FREQ, NODE_GROUP);
}

void loop () {
    if (rf12_recvDone() && rf12_crc == 0 && rf12_len > 0) {
        // save msg in buffer and ack right away to keep things going
        memcpy(doubleBuf, (void*) rf12_data, rf12_len);
        if (RF12_WANTS_ACK)
            rf12_sendStart(RF12_ACK_REPLY, 0, 0, 1);
        rf12_recvDone();
        // this may take some time, depending on the command received
        processmessage(doubleBuf);
    }
}

void processmessage(byte *d) {
    switch (d[0]) {
        case REMOTE_GLCD_CLEAR: 
            glcd.clear(); break;
        case REMOTE_GLCD_SETPIXEL: 
            glcd.setPixel(d[1],d[2],d[3]); break;
        case REMOTE_GLCD_DRAWLINE: 
            glcd.drawLine(d[1],d[2],d[3],d[4],d[5]); break;
        case REMOTE_GLCD_DRAWRECT: 
            glcd.drawRect(d[1],d[2],d[3],d[4],d[5]); break;
        case REMOTE_GLCD_FILLRECT: 
            glcd.fillRect(d[1],d[2],d[3],d[4],d[5]); break;
        case REMOTE_GLCD_DRAWCIRCLE: 
            glcd.drawCircle(d[1],d[2],d[3],d[4]); break;
        case REMOTE_GLCD_FILLCIRCLE: 
            glcd.fillCircle(d[1],d[2],d[3],d[4]); break;
        case REMOTE_GLCD_DRAWTRIANGLE:
            glcd.drawTriangle(d[1],d[2],d[3],d[4],d[5],d[6],d[7]); break;
        case REMOTE_GLCD_FILLTRIANGLE:
            glcd.fillTriangle(d[1],d[2],d[3],d[4],d[5],d[6],d[7]); break;
        case REMOTE_GLCD_DRAWCHAR: 
            glcd.drawChar(d[1],d[2],d[3]); break;
        case REMOTE_GLCD_DRAWSTRING:
            glcd.drawString(d[1],d[2], (char*) &d[3]); break;
        case REMOTE_GLCD_UPDATEAREA:
            glcd.updateDisplayArea(d[1],d[2],d[3],d[4],d[5]); break;
        case REMOTE_GLCD_SETUPDATEAREA:
            glcd.setUpdateArea(d[1],d[2],d[3],d[4],d[5]); break;
        // case REMOTE_GLCD_DRAWBMP:
        case REMOTE_GLCD_REFRESH: 
            glcd.refresh(); break;
        case REMOTE_GLCD_SCROLL: 
            glcd.scroll(d[1],d[2]); break;
        case REMOTE_GLCD_BACKLIGHT: 
            glcd.backLight(d[1]); break;
    }
}
