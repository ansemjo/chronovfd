#define FIL_FWD 5
#define FIL_REV 4
#define FIL_REF 2

#define HV_DATA   8
#define HV_CLOCK  10
#define HV_STROBE 0

void initpin(int pin, int state) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, state);
}

void toggle(int pin) {
  digitalWrite(pin, !digitalRead(pin));
}

void setup() {

  // setup pins for filament driver
  initpin(FIL_FWD, LOW);  // forward
  initpin(FIL_REV, HIGH);  // reverse
  initpin(FIL_REF, LOW); // reference

  // turn on filament
  VREF.CTRLA = 0x03; // set reference voltage to 4.3V
  analogWrite(FIL_REF, 100);

  // setup hv pins
  initpin(HV_DATA, LOW);
  initpin(HV_CLOCK, LOW);
  initpin(HV_STROBE, LOW);

  delay(100);

  // turn on all segments
  digitalWrite(HV_STROBE, LOW);
  shiftOut(HV_DATA, HV_CLOCK, MSBFIRST, 0xff);
  shiftOut(HV_DATA, HV_CLOCK, MSBFIRST, 0xff);
  digitalWrite(HV_STROBE, HIGH);

  delay(1000);

}

#define SEGMENTS 0x1dda
#define Aa 0x0c12
#define An 0x0c02
#define At 0x0c90
#define G1 0x2000
#define G2 0x0001
#define G3 0x0004
#define G4 0x0020
#define G5 0x0200

void filwait() {
  for (int i = 0; i < 3; i++) {
    toggle(FIL_FWD);
    toggle(FIL_REV);
    delay(1);
  }
}

void out(uint16_t data) {
  digitalWrite(HV_STROBE, LOW);
  shiftOut(HV_DATA, HV_CLOCK, MSBFIRST, 0xff & (data >> 8));
  shiftOut(HV_DATA, HV_CLOCK, MSBFIRST, 0xff & (data >> 0));
  digitalWrite(HV_STROBE, HIGH);
}

void loop() {

  filwait();
  out(Aa | G1);
  filwait();
  out(An | G2);
  filwait();
  out(At | G4);
  filwait();
  out(An | G5);

}