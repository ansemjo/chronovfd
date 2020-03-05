#include <TinyWireM.h>
#include <TinyRTClib.h>

#include "segments.h"
#include "hvshift.h"

// jumper pin to enable i2c programming mode
#define PROG 3

// instantiate rtc object
RTC_DS1307 rtc;
static bool rtc_enabled = false;

void setup() {

  hv.begin();  
  pinMode(PROG, INPUT_PULLUP);

  if (digitalRead(PROG) == HIGH) {
    TinyWireM.begin();
    rtc.begin();
    rtc_enabled = true;
  }
  
}

#define DELAY 1

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
    snprintf(buf, 6, "8888:");
  }

  hv.write(G1 | lookup(buf[0]));
  delay(DELAY*6);
  hv.write(G2 | lookup(buf[1]));
  delay(DELAY);
  hv.write(Gd | lookup(buf[4]));
  delay(DELAY);
  hv.write(G3 | lookup(buf[2]));
  delay(DELAY*2);
  hv.write(G4 | lookup(buf[3]));
  delay(DELAY*2);

}

