#include <TinyWireS.h>
#include "segments.h"
#include "hvshift.h"

// jumper pin to enable i2c programming mode
#define JPROG 3
#define progmode() (digitalRead(JPROG) == LOW)

// i2c adresses
#define ADDRESS 0x20
#define RTCADDR 0x68

// buffers for display contents
static volatile char buf[GRIDS + 1];
static volatile char tmp[GRIDS + 1];
static volatile uint8_t reqpos = 0;

void i2cRequester() {

  TinyWireS.send(buf[reqpos]);
  reqpos = (reqpos + 1) % GRIDS;

}

// receive buffer from i2c
void i2cReceiver(uint8_t n) {

  // buf[0] = '8';

  if (n < 1) { return; }

  unsigned i = TinyWireS.receive();
  n--;

  while (n--) {
    buf[i] = TinyWireS.receive();
    i++;
  }

}

void setup() {

  randomSeed(analogRead(0));
  pinMode(JPROG, INPUT_PULLUP);
  hv.begin();

  TinyWireS.begin(ADDRESS);
  TinyWireS.onReceive(i2cReceiver);
  TinyWireS.onRequest(i2cRequester);

  // display i2c address
  snprintf(buf, 6, ";c%02x:", ADDRESS);
  
}

// decode binary coded decimal number
uint8_t bcdDecode(uint8_t b) {
  return ((b / 16 * 10) + (b % 16));
}

void loop() {

  TinyWireS_stop_check();
  hv.text(buf);

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