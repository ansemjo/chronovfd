//#include <SPI.h>
//SPIClass hv(HSPI);

#define LED 5

#define CLOCK   25
#define DATA    26
#define STROBE  27
#define BLANK   18
#define FILSHDN 17
#define HVSHDN  16
#define ENABLE  19

#define pinout(n, i) pinMode(n, OUTPUT); digitalWrite(n, i)

void setup() {

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH); // led on

  pinMode(ENABLE, OUTPUT);
  digitalWrite(ENABLE, LOW); // shifter on

  pinMode(STROBE, OUTPUT);
  digitalWrite(STROBE, LOW); // strobe low

  pinMode(BLANK, OUTPUT);
  digitalWrite(BLANK, LOW); // blank low ~> hv outputs on

  pinMode(FILSHDN, OUTPUT);
  digitalWrite(FILSHDN, LOW); // filament on

  pinMode(HVSHDN, OUTPUT);
  digitalWrite(HVSHDN, HIGH); // hv on

  pinout(CLOCK, LOW);
  pinout(DATA, LOW);

  // write full on to display
  digitalWrite(DATA, HIGH);
  for (unsigned i = 0; i < 16; i++) {
    digitalWrite(CLOCK, HIGH);
    delay(1);
    digitalWrite(CLOCK, LOW);
    delay(1);
  }

  delay(1);
  digitalWrite(STROBE, HIGH);
  delay(1);
  digitalWrite(STROBE, LOW);

}

void loop() {

}
