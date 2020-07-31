#include "segments.h"
#include "hvshift.h"

// buffers for display contents
static char buf[GRIDS + 1];
static uint16_t tmp[GRIDS + 1];
static char text[64];
static unsigned textlen = 0;

void setup() {

  randomSeed(analogRead(0));
  hv.begin();
  
  snprintf(buf, 6, "8888:");
  snprintf(text, 64, "    Hello Wworld");
  textlen = 17;
  
}

// decode binary coded decimal number
uint8_t bcdDecode(uint8_t b) {
  return ((b / 16 * 10) + (b % 16));
}

void loop() {

  //while (true) {
  //  noise();
  //  delay(50);
  //}

  unsigned pos = 0;
  while (pos < textlen) {
    hv.text(text+pos);
    pos++;
    delay(200);
  }

}

// --------------------------------

void milliscounter() {

  long now = (millis() / 10) % 10000;
  snprintf(buf, 6, "%04ld%s", now, (now % 200) < 100 ? ":" : " ");
  hv.text(buf);
  delay(72);

}

void noise() {
  for (unsigned d = 0; d < GRIDS; d++) {
    tmp[d] = random(0xffff) & SEGMENTS;
  }
  hv.raw(tmp);
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
