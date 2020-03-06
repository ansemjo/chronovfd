#include <TinyWireM.h>
#include "segments.h"
#include "hvshift.h"

// jumper pin to enable i2c programming mode
#define JPROG 3
#define progmode() (digitalRead(JPROG) == LOW)

void setup() {

  randomSeed(analogRead(0));
  pinMode(JPROG, INPUT_PULLUP);
  hv.begin();
  TinyWireM.begin();
  
}

// decode binary coded decimal number
uint8_t bcdDecode(uint8_t b) {
  return ((b / 16 * 10) + (b % 16));
}

void loop() {

  static char buf[GRIDS + 1];
  static char tmp[GRIDS + 1];
  if (!progmode()) {

    static uint8_t hours, minutes, seconds;

    // request three bytes from rtc
    TinyWireM.beginTransmission(0x68);
    TinyWireM.send(0);
    TinyWireM.endTransmission();
    TinyWireM.requestFrom(0x68, 3);

    // receive and decode time values
    seconds = bcdDecode(TinyWireM.receive());
    minutes = bcdDecode(TinyWireM.receive());
    hours   = bcdDecode(TinyWireM.receive());
    
    // print formatted time
    snprintf(buf, 6, "%02d%02d%c", hours, minutes, (seconds % 2 == 0) ? ':' : ' ');
    rollover(buf, tmp);
  
  } else {

    snprintf(buf, 6, "~~~~");
    if (millis() % 1000 < 50) {
      buf[4] = '.';
    }
    hv.text(buf);

  }

}

void reveal(const char *buf, unsigned steps, unsigned wait) {

  uint16_t tmp[GRIDS];
  for (unsigned d = 0; d < GRIDS-1; d++) {
    for (unsigned s = 0; s <= steps; s++) {
      if (s == steps) {
        tmp[d] = lookup(buf[d]);
      } else {
        for (unsigned r = d; r < GRIDS-1; r++) {
          tmp[r] = random(0xffff) & SEGMENTS;
        }
      }
      hv.raw(tmp);
      delay(wait);
    }
  }

}

void rollover(const char *a, char *b) {

  bool any = false;
  bool changed[GRIDS];
  uint16_t tmp[GRIDS];
  // check for changed characters
  tmp[GRIDS - 1] = lookup(' ');
  for (unsigned i = 0; i < GRIDS - 1; i++) {
    changed[i] = (a[i] == b[i]) ? false : true;
    any |= changed[i];
    if (!changed[i]) {
      tmp[i] = lookup(b[i]);
    }
  }
  // rollover animation for changed digits
  if (any) {
    for (unsigned s = 0; s < 8; s++) {
      for (unsigned i = 0; i < GRIDS; i++) {
        if (changed[i]) {
          tmp[i] = random(0xffff) & SEGMENTS;
        }
      }
      hv.raw(tmp);
      delay(30);
    }
  }
  // copy a to b
  for (unsigned i = 0; i < GRIDS; i++) {
    b[i] = a[i];
  }
  hv.text(b);

}