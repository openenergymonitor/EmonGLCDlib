// Graphics library for 128x64 display based on ST7565 (JeeLabs Graphics Board)
//
// Originally derived from code by cstone@pobox.com and Limor Fried / Adafruit.
// Massive changes by Steve Evans and Jean-Claude Wippler, see GLCD_ST7565.cpp
// Licensed as LGPL.
//
// 2011-06-01 <jc@wippler.nl>

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include "GLCD_ST7565.h"
#include "GLCD_ST7565_cmds.h"

#define PIN_SID  14
#define PIN_SCLK 4
#define PIN_A0   17
#define PIN_RST  7

//#define LCDUNUSEDSTARTBYTES 4      //JeeLabs GLCD  
#define LCDUNUSEDSTARTBYTES 1      //emonGLCD V2

#define swap(a, b) { byte t = a; a = b; b = t; }

typedef uint8_t PROGMEM prog_uint8_t;

struct FontInfo {
    byte height, width, first, count;
    prog_uint8_t* image;
    prog_uint8_t* widths;
    prog_uint8_t* overflow;
} fontInfo;

// the memory buffer for the LCD
static byte gLCDBuf[1024];

// Switch from fast direct bit flipping to slower Arduino bit writes.
//#define slowSPI

// This makes the library track where changes have occurred and only update the smallest rectangle required
// If you are writing direct to the gLCDBuf you will either need to turn this off, or call setUpdateArea() with the
// area you have been working on so it can track the changes.
#define enablePartialUpdate 1

// moves some of the enablePartialUpdate logic further up the procedure tree. Removing it from setPixel and putting
// it into the higher level routines such as draw/fill rect/circle/triangle. Makes the sketch larger, but faster in
// graphical intensive applications.
#define tradeSizeForSpeed 1  // This needs enablePartialUpdate

#if enablePartialUpdate
static byte xUpdateMin;
static byte xUpdateMax;
static byte yUpdateMin;
static byte yUpdateMax;
#endif

// If the top line is appearing halfway down the screen, try the other mode.
//#define PAGE_FLIP 0x7
#define PAGE_FLIP 0x3		//emonGLCD V2.1

const bool rotate180=false;
// This const switches in the code for operating an LCD panel rotated by 180 degrees.
// The image created is rotated as it is writen into gLCDBuf. This was much faster than performing the required
// bit rotates if it had been implimented in the buffer to panel code.

static void SPIWrite(byte c) {
#ifdef slowSPI
    shiftOut(PIN_SID, PIN_SCLK, MSBFIRST, c);
#else
    for (byte mask = 0x80; mask != 0; mask >>= 1) {
        bitWrite(PORTC, 0, c & mask);
        // this is 15% faster, but it's too fast for this chip...
        //PIND = bit(4);
        //PIND = bit(4);
        // even plain set/clear is too fast, so slow down a bit more
        bitSet(PORTD, 4);
        bitSet(PORTD, 4);
        bitClear(PORTD, 4);
        bitClear(PORTD, 4);
    }
#endif
}

static void st7565_Command(byte c) {
#ifdef slowSPI
    digitalWrite(PIN_A0, LOW);
#else
    bitClear(PORTC, 3);
#endif
    SPIWrite(c);
}

static void st7565_Data(byte c) {
#ifdef slowSPI
    digitalWrite(PIN_A0, HIGH);
#else
    bitSet(PORTC, 3);
#endif
    SPIWrite(c);
}

static void st7565_Set_Brightness(byte val) {
    st7565_Command(CMD_SET_VOLUME_FIRST);
    st7565_Command(CMD_SET_VOLUME_SECOND | (val & 0x3F));
}

static void setPage(byte p) {
    st7565_Command(CMD_SET_PAGE | (p ^ PAGE_FLIP));
}

static void st7565_Init() {
    // set pin directions
    pinMode(PIN_SID,  OUTPUT);
    pinMode(PIN_SCLK, OUTPUT);
    pinMode(PIN_A0,   OUTPUT);
    pinMode(PIN_RST,  OUTPUT);

    digitalWrite(PIN_RST, LOW);
    _delay_ms(500);
    digitalWrite(PIN_RST, HIGH);

    st7565_Command(CMD_SET_BIAS_7);
    st7565_Command(CMD_SET_ADC_NORMAL);
    st7565_Command(CMD_SET_COM_NORMAL);
    st7565_Command(CMD_SET_DISP_START_LINE);
    st7565_Command(CMD_SET_POWER_CONTROL);
    st7565_Command(CMD_SET_RESISTOR_RATIO);
    //st7565_Command(CMD_SET_BIAS_9);

#if enablePartialUpdate
    xUpdateMax = 0;
    yUpdateMax = 0;
    xUpdateMin = LCDWIDTH-1;
    yUpdateMin = LCDHEIGHT-1;
#endif
}

void GLCD_ST7565::begin(byte contrast) {
    st7565_Init();
    st7565_Command(CMD_DISPLAY_ON);
    st7565_Command(CMD_SET_ALLPTS_NORMAL);
    st7565_Set_Brightness(contrast); // strictly speaking this is the contrast
                              // of the LCD panel, the twist on the crystals.
    clear();
}

void GLCD_ST7565::backLight(byte level) {
    analogWrite(5, level);				//emonGLCD V2.1 Digital 5 
}

// the most basic function, set a single pixel
void GLCD_ST7565::setPixel(byte x, byte y, byte color) {
	if (rotate180) {
		x=(LCDWIDTH-1)-x;
		y=(LCDHEIGHT-1)-y;
	}
	if (x < LCDWIDTH && y < LCDHEIGHT) {
		if (color)
			gLCDBuf[x+ (y/8)*128] |= _BV(7-(y%8));
		else
			gLCDBuf[x+ (y/8)*128] &= ~_BV(7-(y%8));

#if enablePartialUpdate
    if (x<xUpdateMin) xUpdateMin=x;
    if (x>xUpdateMax) xUpdateMax=x;
    if (y<yUpdateMin) yUpdateMin=y;
    if (y>yUpdateMax) yUpdateMax=y;
#endif
  }
}

static void mySetPixel(byte x, byte y, byte color) {
#if tradeSizeForSpeed
	if (rotate180) {
		x=(LCDWIDTH-1)-x;
		y=(LCDHEIGHT-1)-y;
	}    
	if (x < LCDWIDTH && y < LCDHEIGHT) {
		if (color) 
			gLCDBuf[x+ (y/8)*128] |=  _BV(7-(y%8));  
		else
			gLCDBuf[x+ (y/8)*128] &= ~_BV(7-(y%8));
		}
#else
    GLCD_ST7565::setPixel(x, y, color);
#endif
}

static void myDrawFont (word x, byte w, const byte* bits, byte xo, byte yo) {
    for (byte j = 0; j < fontInfo.height; ++j) {
        for (byte i = 0; i < w; ++i )
          if (pgm_read_byte((prog_uint8_t*) bits + (x+i) / 8) & bit((x+i) % 8))
              GLCD_ST7565::setPixel(xo+i, yo+j, 1);
        bits += fontInfo.width;
    }
}

void GLCD_ST7565::drawBitmap(byte x, byte y, 
                        const byte *bitmap, byte w, byte h, byte color) {
    for (byte j=0; j<h; j++) {
        for (byte i=0; i<w; i++ ) {
            if (pgm_read_byte(bitmap + i + (j/8)*w) & _BV(j%8))
                setPixel(x+i, y+j, color);
        }
    }
}

byte GLCD_ST7565::setFont (const byte* font) {
    prog_uint8_t* fp = (prog_uint8_t*) font;
    fontInfo.height = pgm_read_byte(fp++);
    fontInfo.width = pgm_read_byte(fp++);
    fontInfo.image = fp;
    fp += fontInfo.height * fontInfo.width;
    fontInfo.first = pgm_read_byte(fp++);
    fontInfo.count = pgm_read_byte(fp++);
    fontInfo.widths = fp;
    if (pgm_read_byte(fp) == 0) {
        fp += fontInfo.count + fontInfo.count + 1;
        fontInfo.overflow = fp;
    } else
        fontInfo.overflow = 0;
    return fontInfo.width;
}

byte GLCD_ST7565::drawChar(byte x, byte y, char c) {
    const struct FontInfo& fi = fontInfo;
    if (c >= fi.first) {
        c -= fi.first;
        if (c < fi.count) {
            byte pix = pgm_read_byte(fi.widths);
            byte gaps = pgm_read_byte(fi.widths+1);
            word pos = 0;
            if (pix == 0) {
                // adjust for offset overflows past 255
                for (byte i = 0; ; ++i) {
                    byte o = pgm_read_byte(fi.overflow + i);
                    if (c <= o)
                        break;
                    pos += 256;
                } 
                // extract offset and gap info for this particular char index
                prog_uint8_t* wp = fi.widths + 2 * c;
                byte off = pgm_read_byte(wp++);
                gaps = pgm_read_byte(wp++);
                byte next = pgm_read_byte(wp);
                // horizontal bitmap position and char width in pixels
                pos += off;
                pix = next - off;
            } else
                pos = c * pix; // mono-spaced fonts
            char pre = (gaps & 0x0F) - 4;
            char post = (gaps >> 4) - 4;
            myDrawFont(pos, pix, fi.image, x + pre, y);
            return pix + post;
        }
    }
    return 0;
}

byte GLCD_ST7565::drawString(byte x, byte y, const char *c) {
    while (*c) {
        byte w = drawChar(x, y, *c++);        
	x += w;
        if (x + w >= LCDWIDTH) {
            x = 0;    // ran out of this line
            if (y + fontInfo.height >= LCDHEIGHT)
                break; // ran out of space :(
            y += fontInfo.height;
        }
    }
    return x;
}

byte GLCD_ST7565::drawString_P(byte x, byte y, const char *c) {
    for (;;) {
        char ch = pgm_read_byte(c++);
        if (ch == 0)
            break;
        byte w = drawChar(x, y, ch);
        x += w;
        if (x + w >= LCDWIDTH) {
            x = 0;    // ran out of this line
            if (y + fontInfo.height >= LCDHEIGHT)
                break; // ran out of space :(
            y += fontInfo.height;
        }
    }
    return x;
}

// bresenham's algorithm - thx wikpedia <-- Pity you didn't quite get it right ;-)
void GLCD_ST7565::drawLine(byte x0, byte y0, byte x1, byte y1, 
                      byte color) {
#if tradeSizeForSpeed            
	if (rotate180) {
		x0=(LCDWIDTH-1)-x0;
		x1=(LCDWIDTH-1)-x1;
		y0=(LCDHEIGHT-1)-y0;
		y1=(LCDHEIGHT-1)-y1;
	}
	if (x0 > xUpdateMax) xUpdateMax = x0;
    if (x0 < xUpdateMin) xUpdateMin = x0;
    if (x1 > xUpdateMax) xUpdateMax = x1;
    if (x1 < xUpdateMin) xUpdateMin = x1;
                                      
    if (y0 > yUpdateMax) yUpdateMax = y0;
    if (y0 < yUpdateMin) yUpdateMin = y0;
    if (y1 > yUpdateMax) yUpdateMax = y1;
    if (y1 < yUpdateMin) yUpdateMin = y1;
	
	if (rotate180) {
		x0=(LCDWIDTH-1)-x0;
		x1=(LCDWIDTH-1)-x1;
		y0=(LCDHEIGHT-1)-y0;
		y1=(LCDHEIGHT-1)-y1;
	}
	
#endif

    byte steep = abs(y1-y0) > abs(x1-x0);
    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }
  
    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }
  
    byte dx = x1 - x0, dy = abs(y1 - y0);
    char err = dx / 2, ystep = y0 < y1 ? 1 : -1;
  
    while (x0 <= x1) {
        if (steep)
            mySetPixel(y0, x0, color);
        else
            mySetPixel(x0, y0, color);
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
        ++x0;
    }
}

// draw triangle
void GLCD_ST7565::drawTriangle(byte x0, byte y0, byte x1, byte y1, byte x2, byte y2, byte color) {
    drawLine(x0,y0,x1,y1,color);
    drawLine(x1,y1,x2,y2,color);
    drawLine(x2,y2,x0,y0,color);
}

static void drawTriangleLine(byte x0, byte y0, byte x1, byte y1, byte firstLine, byte *points, byte color) {
    byte steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }

    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }

    byte dx = x1 - x0, dy = abs(y1 - y0);
    char err = dx / 2, ystep = y0 < y1 ? 1 : -1;

    while (x0 <= x1) {
        if (steep) {
            if (!firstLine)
                GLCD_ST7565::drawLine(y0, x0, points[x0], x0, color);
            else if (x0 >= 0 && x0 < LCDHEIGHT)
                points[x0] = y0;
        } else {
            if (!firstLine)
                GLCD_ST7565::drawLine(x0, y0, points[y0], y0, color);
            else if (y0 >= 0 && y0 < LCDHEIGHT)
                points[y0] = x0;
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
        ++x0;
    }
}

void GLCD_ST7565::fillTriangle(byte x0, byte y0, byte x1, byte y1, byte x2, byte y2, byte color) {
    byte points[LCDHEIGHT]; // 64 bytes taken to store line points for fill.
    // first we need to find the highest and lowest point
    // a little unrolled bubble will do for 3 points
    if (y2 < y1) { swap(y1,y2); swap(x1,x2); }
    if (y1 < y0) { swap(y1,y0); swap(x1,x0); }
    if (y2 < y1) { swap(y1,y2); swap(x1,x2); }
    // Now y0 is the top and y2 is the bottom.
        
    // The longest(vertical) line generate the entries in points array
    // the other lines do the drawing
    drawTriangleLine(x0, y0, x2, y2, 1, points, color);
    drawTriangleLine(x0, y0, x1, y1, 0, points, color);
    drawTriangleLine(x1, y1, x2, y2, 0, points, color);
}

// filled rectangle
void GLCD_ST7565::fillRect(byte x, byte y, byte w, byte h, byte color) {
    // stupidest version - just pixels - but fast with internal buffer!
#if tradeSizeForSpeed
	if (rotate180) {
		byte x1=(LCDWIDTH-1)-x-(w-1);
		byte y1=(LCDHEIGHT-1)-y-(h-1);
		byte x2=(LCDWIDTH-1)-x;
		byte y2=(LCDHEIGHT-1)-y;
		if (x1 < xUpdateMin) xUpdateMin = x1;
		if (y1 < yUpdateMin) yUpdateMin = y1;
		if (x2 > xUpdateMax) xUpdateMax = x2;
		if (y2 > yUpdateMax) yUpdateMax = y2;
	}
	else {
		byte x1=x;
		byte x2=x+w-1;
		byte y1=y;
		byte y2=y+h-1;

		if (x1 < xUpdateMin) xUpdateMin = x1;
		if (y1 < yUpdateMin) yUpdateMin = y1;
		if (x2 > xUpdateMax) xUpdateMax = x2;
		if (y2 > yUpdateMax) yUpdateMax = y2;
	}
#endif


	for (byte i = x; i <x+w; i++) {
		for (byte j = y; j <y+h; ++j) mySetPixel(i, j, color);
	}
}

// draw a rectangle
void GLCD_ST7565::drawRect(byte x, byte y, byte w, byte h, byte color) {
    // stupidest version - just pixels - but fast with internal buffer!

#if tradeSizeForSpeed
	if (rotate180) {
		byte x1=(LCDWIDTH-1)-x-(w-1);
		byte y1=(LCDHEIGHT-1)-y-(h-1);
		byte x2=(LCDWIDTH-1)-x;
		byte y2=(LCDHEIGHT-1)-y;
		if (x1 < xUpdateMin) xUpdateMin = x1;
		if (y1 < yUpdateMin) yUpdateMin = y1;
		if (x2 > xUpdateMax) xUpdateMax = x2;
		if (y2 > yUpdateMax) yUpdateMax = y2;
	}
	else {
		byte x1=x;
		byte x2=x+w-1;
		byte y1=y;
		byte y2=y+h-1;

		if (x1 < xUpdateMin) xUpdateMin = x1;
		if (y1 < yUpdateMin) yUpdateMin = y1;
		if (x2 > xUpdateMax) xUpdateMax = x2;
		if (y2 > yUpdateMax) yUpdateMax = y2;
	}
#endif

    for (byte i=x; i<x+w; i++) {
        mySetPixel(i, y, color);
        mySetPixel(i, y+h-1, color);
    }

    for (byte i=y; i<y+h; i++) {
        mySetPixel(x, i, color);
        mySetPixel(x+w-1, i, color);
    } 
}

// draw a circle outline
void GLCD_ST7565::drawCircle(byte x0, byte y0, byte r, byte color) {
#if tradeSizeForSpeed
	if (rotate180) {
		x0=(LCDWIDTH-1)-x0;
		y0=(LCDHEIGHT-1)-y0;
	}
    if (x0-r < xUpdateMin) xUpdateMin = x0-r;
    if (y0-r < yUpdateMin) yUpdateMin = y0-r;
    if (x0+r > xUpdateMax) xUpdateMax = x0+r;
    if (y0+r > yUpdateMax) yUpdateMax = y0+r;   
	if (rotate180) {
		x0=(LCDWIDTH-1)-x0;
		y0=(LCDHEIGHT-1)-y0;
	}
#endif

    char f = 1 - r;
    char ddF_x = 1;
    char ddF_y = -2 * r;
    char x = 0;
    char y = r;
  
    mySetPixel(x0, y0+r, color);
    mySetPixel(x0, y0-r, color);
    mySetPixel(x0+r, y0, color);
    mySetPixel(x0-r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        mySetPixel(x0 + x, y0 + y, color);
        mySetPixel(x0 - x, y0 + y, color);
        mySetPixel(x0 + x, y0 - y, color);
        mySetPixel(x0 - x, y0 - y, color);
    
        mySetPixel(x0 + y, y0 + x, color);
        mySetPixel(x0 - y, y0 + x, color);
        mySetPixel(x0 + y, y0 - x, color);
        mySetPixel(x0 - y, y0 - x, color);
    }
}

void GLCD_ST7565::fillCircle(byte x0, byte y0, byte r, byte color) {
#if tradeSizeForSpeed
	if (rotate180) {
		x0=(LCDWIDTH-1)-x0;
		y0=(LCDHEIGHT-1)-y0;
	}
    if (x0-r < xUpdateMin) xUpdateMin = x0-r;
    if (y0-r < yUpdateMin) yUpdateMin = y0-r;
    if (x0+r > xUpdateMax) xUpdateMax = x0+r;
    if (y0+r > yUpdateMax) yUpdateMax = y0+r;   
	if (rotate180) {
		x0=(LCDWIDTH-1)-x0;
		y0=(LCDHEIGHT-1)-y0;
	}	
#endif

    char f = 1 - r;
    char ddF_x = 1;
    char ddF_y = -2 * r;
    char x = 0;
    char y = r;

    for (byte i=y0-r; i<=y0+r; i++)
        setPixel(x0, i, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
  
        for (byte i=y0-y; i<=y0+y; i++) {
            mySetPixel(x0+x, i, color);
            mySetPixel(x0-x, i, color);
        } 
        for (byte i=y0-x; i<=y0+x; i++) {
            mySetPixel(x0+y, i, color);
            mySetPixel(x0-y, i, color);
        }    
    }
}

void GLCD_ST7565::refresh() {
#if enablePartialUpdate
    if (xUpdateMin<=xUpdateMax) {
        for (byte p = yUpdateMin>>3; p <= yUpdateMax>>3; ++p) {
            setPage(p);
            st7565_Command(CMD_SET_COLUMN_UPPER |
                            (((xUpdateMin+LCDUNUSEDSTARTBYTES) >> 4) & 0x0F));
            st7565_Command(CMD_SET_COLUMN_LOWER |
                            ((xUpdateMin+LCDUNUSEDSTARTBYTES) & 0x0F));
            st7565_Command(CMD_RMW);
            // With column offset set to 4 you don't need or want this.
            for (byte c = xUpdateMin; c <=xUpdateMax; c++)
                st7565_Data(gLCDBuf[(128*p)+c]);
        }
        // after the screen update, reset the redraw region to nothing.
        xUpdateMax = 0;
        yUpdateMax = 0;
        xUpdateMin = LCDWIDTH-1;
        yUpdateMin = LCDHEIGHT-1;
    }
#else
    for (byte p = 0; p < 8; p++) {
        setPage(p);
        st7565_Command(CMD_SET_COLUMN_LOWER |
                        (LCDUNUSEDSTARTBYTES & 0x0F));
        st7565_Command(CMD_SET_COLUMN_UPPER |
                        ((LCDUNUSEDSTARTBYTES >> 4) & 0x0F));
        st7565_Command(CMD_RMW);
        // With column offset set to 4 you don't need or want this.
        for (byte c = 0; c < 128; c++)
            st7565_Data(gLCDBuf[(128*p)+c]);
    }
#endif
}

void GLCD_ST7565::setUpdateArea(byte x0, byte y0, byte x1, byte y1, byte allowReduction) {
#if enablePartialUpdate
    if (x0 == 0xFF && allowReduction) {
        xUpdateMax = 0;           // reset area to nothing
        yUpdateMax = 0;
        xUpdateMin = LCDWIDTH-1;
        yUpdateMin = LCDHEIGHT-1;     
    } else {
        if (rotate180) {
			x0=(LCDWIDTH-1)-x0;
			x1=(LCDWIDTH-1)-x1;
			y0=(LCDHEIGHT-1)-y0;
			y1=(LCDHEIGHT-1)-y1;
		}
		
		if (x0 > x1) swap(x0,x1);
        if (y0 > y1) swap(y0,y1);
    
        if (allowReduction) {
            xUpdateMin = x0;
            xUpdateMax = x1;
            yUpdateMin = y0;
            yUpdateMax = y1;
        } else {
            if (x0 < xUpdateMin) xUpdateMin = x0;
            if (x1 > xUpdateMax) xUpdateMax = x1;
            if (y0 < yUpdateMin) yUpdateMin = y0;
            if (y1 > yUpdateMax) yUpdateMax = y1;
        }
    }
#endif
}

void GLCD_ST7565::updateDisplayArea(byte x0,byte y0,byte x1,byte y1, byte reset) {
#if enablePartialUpdate
    if (x0 <= x1 && y0 <= y1) {
        for (byte p = y0>>3; p <= y1>>3; ++p) {
            setPage(p);
            st7565_Command(CMD_SET_COLUMN_UPPER |
                            (((x0+LCDUNUSEDSTARTBYTES) >> 4) & 0x0F));
            st7565_Command(CMD_SET_COLUMN_LOWER |
                            ((x0+LCDUNUSEDSTARTBYTES) & 0x0F));
            st7565_Command(CMD_RMW);
            for (byte c = x0; c <= x1; ++c)
                st7565_Data(gLCDBuf[128*p+c]);
        }
    }
    if (reset || (x0 <= xUpdateMin && x1 >= xUpdateMax &&
                  y0 <= yUpdateMin && y1 >= yUpdateMax)) {
        // if reset set, or the area painted covers the current region,
        // then clear the partial update region
        xUpdateMax = 0;           
        yUpdateMax = 0;
        xUpdateMin = LCDWIDTH-1;
        yUpdateMin = LCDHEIGHT-1;     
    }
#endif
}

// clear everything
void GLCD_ST7565::clear() {
    memset(gLCDBuf, 0x00, sizeof gLCDBuf);
#if enablePartialUpdate
    xUpdateMin = 0; // set the partial update region to the whole screen
    yUpdateMin = 0;
    xUpdateMax = LCDWIDTH-1;
    yUpdateMax = LCDHEIGHT-1;
#endif
}

static void st7565_scrollUp(byte y) {
#if enablePartialUpdate
    xUpdateMin = 0;   // set the partial update region to the whole screen
    yUpdateMin = 0;
    xUpdateMax = LCDWIDTH-1;
    yUpdateMax = LCDHEIGHT-1;
#endif
    if (y>7) {     // easy one, whole line scroll. This routine is used to do the whole line jumps.
        for (byte l = 0; l < 8-(y>>3); ++l) {
            byte* p = gLCDBuf +(l * 128);
            for (byte x = 0; x < LCDWIDTH; ++x) {
                *(p+x) = *(p+(y>>3)*128+x);
            }
        }
        for (byte l = 8-(y>>3); l < 8; ++l) {
            byte* p = gLCDBuf +(l * 128);
            for (byte x = 0; x < LCDWIDTH; ++x)
                *(p+x) = 0;
        }
    }
    byte bits = y&7;
    if (bits) { // And now the fine scroll bit
        for (byte l = 0; l < 7; ++l) {
            byte* p = gLCDBuf +(l * 128);
            for (byte x = 0; x < LCDWIDTH; ++x) {
                byte top = *(p+x)<<bits;
                byte bottom = *(p+x+128)>>(8-bits);
                *(p+x) = top | bottom;                                
            }           
        }   
        byte* p = gLCDBuf +(7 * 128);
        for (byte x = 0; x < LCDWIDTH; ++x)
            *(p+x) = *(p+x)<<bits;
    }
}

static void st7565_scrollDown(byte y) {
#if enablePartialUpdate
    xUpdateMin=0;   // set the partial update region to the whole screen
    yUpdateMin=0;
    xUpdateMax=LCDWIDTH-1;
    yUpdateMax=LCDHEIGHT-1;
#endif
    if (y>7) {     // easy one, whole line scroll.
        for (byte l = 7; l >= (y>>3); --l) {
            byte* p = gLCDBuf + (l * 128);
            for (byte x = 0; x < LCDWIDTH; ++x)
                *(p+x) = *(p-(y>>3)*128+x);
        }
        for (byte l = 0; l < (y>>3); ++l) {
            byte* p = gLCDBuf +(l * 128);
            for (byte x = 0; x < LCDWIDTH; ++x)
                *(p+x) = 0;
        }
    }
    if ((y&7)!=0) {         // And now the fine scroll bit
        byte bits=y&7;
        for (byte l=7;l>0;l--) {
            byte* p = gLCDBuf +(l * 128);
            for (byte x = 0; x < LCDWIDTH; ++x) {
                byte bottom = *(p+x)>>bits;
                byte top = *(p+x-128)<<(8-bits);
                *(p+x) = top | bottom;                                
            }           
        }   
        byte* p = gLCDBuf;
        for (byte x=0;x<LCDWIDTH;x++)
            *(p+x)=*(p+x)>>bits;
    }
}

static void st7565_scrollLeft(byte x) {
#if enablePartialUpdate
    xUpdateMin=0;   // set the partial update region to the whole screen
    yUpdateMin=0;
    xUpdateMax=LCDWIDTH-1;
    yUpdateMax=LCDHEIGHT-1;
#endif
    for (byte l=0;l<=7;l++) {
        byte* p = gLCDBuf +(l * 128);
        for (byte b = 0; b < LCDWIDTH-x; ++b)
            *(p+b) = *(p+b+x);
        for (byte b = LCDWIDTH-x; b < LCDWIDTH; ++b)
            *(p+b) = 0;
    }
}

static void st7565_scrollRight(byte x) {
#if enablePartialUpdate
    xUpdateMin = 0;   // set the partial update region to the whole screen
    yUpdateMin = 0;
    xUpdateMax = LCDWIDTH-1;
    yUpdateMax = LCDHEIGHT-1;
#endif
    for (byte l = 0; l < 8; ++l) {
        byte* p = gLCDBuf + (l * 128);
        for (byte b = LCDWIDTH-1; b >= x; --b)
            *(p+b) = *(p+b-x);
        for (byte b = 0; b < x; ++b)
            *(p+b) = 0;
    }
}

void GLCD_ST7565::scroll(byte direction, byte pixels) {
    if (rotate180) {
		switch (direction) {
			case SCROLLDOWN:  st7565_scrollUp(pixels); break;
			case SCROLLUP:    st7565_scrollDown(pixels); break;
			case SCROLLRIGHT: st7565_scrollLeft(pixels); break;
			case SCROLLLEFT:  st7565_scrollRight(pixels); break;
		}
	}
	else {
		switch (direction) {
			case SCROLLUP:    st7565_scrollUp(pixels); break;
			case SCROLLDOWN:  st7565_scrollDown(pixels); break;
			case SCROLLLEFT:  st7565_scrollLeft(pixels); break;
			case SCROLLRIGHT: st7565_scrollRight(pixels); break;
		}
	}
}
