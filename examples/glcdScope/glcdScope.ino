// Very simple 100 KHz DSO scope, using the Graphics Board as display device.
// 2010-11-19 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>
#include <GLCD_ST7565.h>

GLCD_ST7565 glcd;
byte samples[128];

static void sample() {
  for (byte i = 0; i < 128; ++i) {
    loop_until_bit_is_set(ADCSRA, ADIF);
    bitSet(ADCSRA, ADIF);
    samples[i] = ADCH;
  }
}

void setup () {
  Serial.begin(57600);
  Serial.println("\n[glcdScope]");
  
  glcd.begin();
  glcd.backLight(255);
  
  analogRead(2); // run once to set up the ADC
  
  bitSet(ADMUX, ADLAR);   // left adjust result

  // the ADC clock will determine the sample rate
  ADCSRA = (ADCSRA & ~7) | 3; // values 2..7 are ok
  
  ADCSRB &= ~7;           // free-running
  bitSet(ADCSRA, ADATE);  // auto trigger
  bitSet(ADCSRA, ADSC);   // start conversion
}

void loop () {
  glcd.clear();
  
  // sample quickly with interrupts disabled
  cli();
  sample();
  sei();

  for (byte i = 0; i < 128; ++i)
    glcd.setPixel(i, 63 - samples[i] / 4, 1);

  glcd.refresh();
  delay(200);
}
