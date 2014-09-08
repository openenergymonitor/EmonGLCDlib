// Show incoming Room Node messages on the Graphics Board
// 2010-11-15 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// Note, this sketch requires the GLCDlib library from JeeLabs
// http://jeelabs.net/projects/cafe/wiki/GLCDlib

#include <GLCD_ST7565.h>
#include <JeeLib.h>
#include "utility/font_clR6x6.h"

class GraphicsBoard : public GLCD_ST7565, public Print {
    byte x, y, dirty;
    MilliTimer refreshTimer;
    
    void newline() {
        x = 0;
        if (y >= 54)
            scroll(SCROLLUP, 6);
        else
            y += 6;
    }
    
public:
    GraphicsBoard () : x (0), y (0), dirty (0) {}
    
    void poll(byte rate =100) {
        if (refreshTimer.poll(rate) && dirty) {
            refresh();
            dirty = 0;
        }
    }
    
    virtual size_t write(byte c) {
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
byte index;
char line[25];

struct {
    byte light;     // light sensor: 0..255
    byte moved :1;  // motion detector: 0..1
    byte humi  :7;  // humidity: 0..100
    int temp   :10; // temperature: -500..+500 (tenths)
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

void setup () {
    Serial.begin(57600);
    Serial.println("\n[glcdNode]");
    //rf12_initialize(1, RF12_868MHZ, 5);
    rf12_config();
    glcd.begin();
    glcd.backLight(255);
    glcd.setFont(font_clR6x6);
    glcd.println(" <<< glcdNode.pde >>>");
}

void loop () {
    glcd.poll();
    
    // assume all 4-byte packets come from Room Nodes ...
    if (rf12_recvDone() && rf12_crc == 0) {
        if (++index > 99)
            index = 0;
        if (rf12_len == sizeof payload) {
            memcpy(&payload, (void*) rf12_data, sizeof payload);
            sprintf(line, "%02d%c%2d%c%3d.%dC%3d%%%4d*", 
                            index, payload.lobat ? '!' : ' ',
                            rf12_hdr & 0x1F, payload.moved ? ':' : '#',
                            payload.temp / 10, payload.temp % 10,
                            payload.humi, payload.light);
            
            Serial.println(line);
            glcd.println(line);
        }
    }
}
