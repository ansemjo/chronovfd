#include <TinyWireM.h>
#include <TinyWireS.h>
#include "segments.h"
#include "hvshift.h"

// jumper pin to enable i2c mode
#define MODEJUMPER 3
static bool clock = false;

// i2c addresses
#define ADDRESS 0x20
#define RTCADDR 0x68

// buffers for display contents
static char buf[GRIDS + 1];
static char tmp[GRIDS + 1];
static uint8_t reqpos = 0;

void setup() {

  // seed the rng for effects
  randomSeed(analogRead(0));

  pinMode(MODEJUMPER, INPUT_PULLUP);
  if (digitalRead(MODEJUMPER) == LOW) {
    
    // setup as i2c peripheral
    clock = false;
    TinyWireS.begin(ADDRESS);
    TinyWireS.onReceive(i2cReceiver);
    TinyWireS.onRequest(i2cRequester);
    snprintf(buf, 6, "8888:");
  
  } else {
    
    // setup as clock with ds3231 rtc
    clock = true;
    TinyWireM.begin();
  
  }
  
  // setup hv5812 driver
  hv.begin();
  
}

// decode binary coded decimal number
uint8_t bcdDecode(uint8_t b) {
  return ((b / 16 * 10) + (b % 16));
}

// main loop
void loop() {

  if (clock) {

    static uint8_t hours, minutes, seconds;

    // request three bytes from rtc
    TinyWireM.beginTransmission(RTCADDR);
    TinyWireM.send(0);
    TinyWireM.endTransmission();
    TinyWireM.requestFrom(RTCADDR, 3);

    // receive and decode time values
    seconds = bcdDecode(TinyWireM.receive());
    minutes = bcdDecode(TinyWireM.receive());
    hours   = bcdDecode(TinyWireM.receive());
    
    // print formatted time
    snprintf(buf, 6, "%02d%02d%c", hours, minutes, (seconds % 2 == 0) ? ':' : ' ');
    rollover(buf, tmp);
  
  } else {

    TinyWireS_stop_check();
    hv.text(buf);

  }

}

// send out data from buffer
void i2cRequester() {
  TinyWireS.send(buf[reqpos]);
  reqpos = (reqpos + 1) % GRIDS;
}

// fill buffer from i2c data
void i2cReceiver(uint8_t n) {
  // early-out if zero length
  if (n < 1) { return; }
  // get index from first byte
  unsigned i = TinyWireS.receive();
  n--;
  // while (data) write buf
  while (n--) {
    buf[i] = TinyWireS.receive();
    i++;
  }
}

// reveal text from random noise
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

// scramble-animation for new digits
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
