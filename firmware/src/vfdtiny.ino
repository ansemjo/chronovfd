#include <TinyWireM.h>
#include <TinyRTClib.h>

#include "segments.h"

#define CLCK 7
#define DATA 9
#define STRB 8
#define PROG 3

#define toggle(pin) digitalWrite(pin, !digitalRead(pin))
#define strobe(pin) toggle(pin); toggle(pin)

RTC_DS1307 rtc;
static bool rtc_enabled = false;

void setup() {

  pinMode(STRB, OUTPUT);
  pinMode(CLCK, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(PROG, INPUT_PULLUP);

  if (digitalRead(PROG) == HIGH) {
    TinyWireM.begin();
    rtc.begin();
    rtc_enabled = true;
  }
  
}


void hvwrite(uint16_t data) {

  digitalWrite(CLCK, LOW);
  digitalWrite(STRB, LOW);

  for (unsigned i = 0; i < 14; i++) {

    digitalWrite(DATA, (data >> i) & 1);
    strobe(CLCK);

  }

  strobe(STRB);
  delay(2);

}

void loop() {

  static char buf[32];
  if (rtc_enabled) {
    static uint8_t hour, minute, sec;
    DateTime now = rtc.now();
    hour = now.hour();
    minute = now.minute();
    sec = now.second();
    snprintf(buf, 6, "%02d%02d%c", hour, minute, (sec % 2 == 0) ? ':' : ' ');
  } else {
    snprintf(buf, 6, "Prog");
  }

  hvwrite(G1 | lookup(buf[0]));
  hvwrite(G2 | lookup(buf[1]));
  hvwrite(G3 | lookup(buf[2]));
  hvwrite(G4 | lookup(buf[3]));
  hvwrite(Gd | lookup(buf[4]));

}

