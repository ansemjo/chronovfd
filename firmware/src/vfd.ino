#include <TinyWireM.h>
//#include <TinyRTClib.h>

#include "segments.h"
#include "hvshift.h"

// jumper pin to enable i2c programming mode
#define PROG 3

// background refreshing
volatile static int __isr_digit = 0;
volatile static uint16_t grids[GRIDS] = { G1, G2, G3, G4, Gd };
volatile static uint16_t display[GRIDS];

ISR(TIMER1_COMPA_vect) {
  hv.write(grids[__isr_digit] | display[__isr_digit]);
  __isr_digit = (__isr_digit + 1) % GRIDS;
}


// instantiate rtc object
//RTC_DS1307 rtc;
static bool rtc_enabled = false;

void setup() {

  hv.begin();
  // enable background timer
  for (int i = 0; i < GRIDS; i++) { display[i] = 0; }
  TCCR1A = (1 << WGM11); // ctc
  TCCR1B = (0 << CS12) | (1 << CS11) | (0 << CS10); // prescaling 1024
  OCR1A = 2; // ca 1 kHz
  TIMSK1 = (1 << OCIE1A); // enable compare isr
  interrupts();

  pinMode(PROG, INPUT_PULLUP);

  if (digitalRead(PROG) == HIGH) {
    TinyWireM.begin();
    //rtc.begin();
    rtc_enabled = true;
  }
  
}

#define DELAY 1

byte bcdToDec(byte val) {               // Convert binary coded decimal to normal decimal numbers
  return ((val / 16 * 10) + (val % 16));
}

void loop() {

  for (int i = 0; i < GRIDS; i++) {
    //display[i] = 0;
  }

  static char buf[32];
  if (rtc_enabled) {
    static uint8_t hour, minute, sec;
    TinyWireM.beginTransmission(0x68);
    TinyWireM.send(0);
    TinyWireM.endTransmission();
    TinyWireM.requestFrom(0x68, 3);
    //DateTime now = rtc.now();
    sec = bcdToDec(TinyWireM.receive()); //00; //now.second();
    minute = bcdToDec(TinyWireM.receive()); //15; //now.minute();
    hour = bcdToDec(TinyWireM.receive());// 20; //now.hour();
    snprintf(buf, 6, "%02d%02d%c", hour, minute, (sec % 2 == 0) ? ':' : ' ');
  } else {
    snprintf(buf, 6, "8888:");
  }

  for (int i = 0; i < GRIDS; i++) {
    display[i] = lookup(buf[i]);
  }

  // hv.write(G1 | lookup(buf[0]));
  // delay(DELAY*6);
  // hv.write(G2 | lookup(buf[1]));
  // delay(DELAY);
  // hv.write(Gd | lookup(buf[4]));
  // delay(DELAY);
  // hv.write(G3 | lookup(buf[2]));
  // delay(DELAY*2);
  // hv.write(G4 | lookup(buf[3]));
  // delay(DELAY*2);

}

