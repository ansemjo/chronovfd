#include <Arduino.h>
#include "segments.h"
#include "hvshift.h"

// brightness correction with repeated digits
const static unsigned digits[] = { 0, 1, 4, 2, 3 };
//const static unsigned digits[] = { 0, 0, 0, 1, 1, 4, 2, 3 }; // brightness correction for hendriks display
const static uint16_t grids[GRIDS] = { G1, G2, G3, G4, Gd };
volatile static uint16_t display[GRIDS];

// background refreshing with timer interrupts
volatile static int isr_counter = 0;
ISR(TIMER1_COMPA_vect) {
  hv.write(grids[digits[isr_counter]] | display[digits[isr_counter]]);
  isr_counter = (isr_counter + 1) % (sizeof(digits)/sizeof(digits[0]));
}

// initialize pin directions and setup timer
void HV::begin() {

  HVSPI.begin(25, 26, 0, 1);
  hv.clear();

}

// write data to shift register and latch to outputs
void HV::write(uint16_t data) {

  // bring strobe low to prevent latching
  digitalWrite(HV_STROBE, LOW);

  // shift out data bits on rising clock edge
  for (unsigned i = 0; i < SHIFTWIDTH; i++) {
    digitalWrite(HV_CLOCK, LOW);
    digitalWrite(HV_DATA, (data >> i) & 1);
    digitalWrite(HV_CLOCK, HIGH);
  }

  // strobe to latch shifted bits to outputs
  digitalWrite(HV_STROBE, HIGH);

}

// store a looked-up string in display buffer
void HV::text(const char *str) {
  for (unsigned i = 0; i < GRIDS; i++) {
    display[i] = lookup(str[i]);
  }
}

// store raw data in display buffer
void HV::raw(const uint16_t *buf) {
  for (unsigned i = 0; i < GRIDS; i++) {
    display[i] = buf[i];
  }
}

// clear buffer contents
void HV::clear() {
  for (unsigned i = 0; i < GRIDS; i++) {
    display[i] = 0;
  }
}

HV hv;
