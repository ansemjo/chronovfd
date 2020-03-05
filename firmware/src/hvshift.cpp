#include <Arduino.h>
#include "segments.h"
#include "hvshift.h"

void HV::begin() {
  // define appropriate pins as outputs
  pinMode(HV_CLOCK,  OUTPUT);
  pinMode(HV_DATA,   OUTPUT);
  pinMode(HV_STROBE, OUTPUT);
}

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

void HV::clear() {
  hv.write(0);
}

HV hv;