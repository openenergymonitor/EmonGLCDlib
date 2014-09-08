// Very simple Spectrum Analyzer, using the Graphics Board as display device.
// 2010-11-19 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>
#include <GLCD_ST7565.h>

extern int fix_fft(char[], char[], int, int);

GLCD_ST7565 glcd;
char samples[256], im[256];
byte rescans;
word avg = 512;

void setup () {
    Serial.begin(57600);
    Serial.println("\n[glcdSpectrum50]");    
    glcd.begin();
    glcd.backLight(255);
}

void loop () {
  if (++rescans >= 100) {
    rescans = 0;
    glcd.clear();
  }
  
  long sum = 0;
  for (word i = 0; i < 256; ++i) {
    int v = analogRead(2);
    sum += v;
    v -= avg;
    samples[i] = v < -127 ? -127 : v > 127 ? 127 : v;
  }
  avg = sum >> 8;

  // perform a 2^8, i.e. 256-point Fast Fourier Transform
  memset(im, 0, sizeof im);
  fix_fft(samples, im, 8, 0);
  
  // display first half of the results
  for (byte i = 0; i < 128; ++i)
    glcd.setPixel(i, 63 - samples[i], 1);

  glcd.refresh();
}
