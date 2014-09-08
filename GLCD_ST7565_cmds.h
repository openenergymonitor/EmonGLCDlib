// Graphics library for 128x64 display based on ST7565 (JeeLabs Graphics Board)
//
// Originally derived from code by cstone@pobox.com and Limor Fried / Adafruit.
// Massive changes by Steve Evans and Jean-Claude Wippler, see GLCD_ST7565.cpp
// Licensed as LGPL.
//
// 2011-06-01 <jc@wippler.nl>

#define CMD_DISPLAY_OFF         0xAE        //  1
#define CMD_DISPLAY_ON          0xAF        //  1

#define CMD_SET_DISP_START_LINE 0x40        //  2
#define CMD_SET_PAGE            0xB0        //  3

#define CMD_SET_COLUMN_UPPER    0x10        //  4
#define CMD_SET_COLUMN_LOWER    0x00        //  4

#define CMD_SET_ADC_NORMAL      0xA0        //  8
#define CMD_SET_ADC_REVERSE     0xA1        //  8

#define CMD_SET_DISP_NORMAL     0xA6        //  9
#define CMD_SET_DISP_REVERSE    0xA7        //  9

#define CMD_SET_ALLPTS_NORMAL   0xA4        // 10
#define CMD_SET_ALLPTS_ON       0xA5        // 10
#define CMD_SET_BIAS_9          0xA2        // 11
#define CMD_SET_BIAS_7          0xA3        // 11

#define CMD_RMW                 0xE0        // 12
#define CMD_RMW_CLEAR           0xEE        // 13
#define CMD_INTERNAL_RESET      0xE2        // 14
#define CMD_SET_COM_NORMAL      0xC0        // 15
#define CMD_SET_COM_REVERSE     0xC8        // 15
#define CMD_SET_POWER_CONTROL   0x2F        // 16
#define CMD_SET_RESISTOR_RATIO  0x24        // 17
#define CMD_SET_VOLUME_FIRST    0x81        // 18
#define CMD_SET_VOLUME_SECOND   0x00        // 18
#define CMD_SET_STATIC_OFF      0xAC        // 19
#define CMD_SET_STATIC_ON       0xAD        // 19
#define CMD_SET_STATIC_REG      0x00        // 19
#define CMD_NOP                 0xE3        // 22
#define CMD_TEST                0xF0        // 23