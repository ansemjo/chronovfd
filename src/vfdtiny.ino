#include <TinyWireM.h>
#include <TinyRTClib.h>

#define CLCK 1
#define DATA 2
#define STRB 3

#define toggle(pin) digitalWrite(pin, !digitalRead(pin))
#define strobe(pin) toggle(pin); toggle(pin)

RTC_DS1307 rtc;
static bool rtc_enabled = false;

void setup() {

  pinMode(STRB, OUTPUT);
  pinMode(CLCK, OUTPUT);
  pinMode(DATA, OUTPUT);

  if (analogRead(4) < 750) { 
    TinyWireM.begin();
    rtc.begin();
    rtc_enabled = true;
  }
  
}

// digits
#define d1 (1 << 13)
#define d2 (1 << 12)
#define dd (1 << 11)
#define d3 (1 << 10)
#define d4 (1 <<  9)

// segments
#define a  (1 << 0)
#define b  (1 << 2)
#define c  (1 << 5)
#define d  (1 << 3)
#define e  (1 << 6)
#define f  (1 << 1)
#define g  (1 << 7)
#define dt (1 << 4)
#define db (1 << 8)

uint16_t lookup(char ch) {
  switch (ch) {
    case '0': return a|b|c|d|e|f;
    case '1': return b|c;
    case '2': return a|b|g|e|d;
    case '3': return a|b|c|d|g;
    case '4': return f|g|b|c;
    case '5': return a|f|g|c|d;
    case '6': return a|f|e|d|c|g;
    case '7': return a|b|c;
    case '8': return a|b|c|d|e|f|g;
    case '9': return a|b|c|d|g|f;
    case ':': return dt|db;
    case '.': return db;
    case ',': return dt;
    case 'H': return b|c|e|f|g;
    case 'L': return d|e|f;
    case 'c': return d|e|g;
    case 'd': return b|c|d|e|g;
    case 'P': return a|b|e|f|g;
    case 'r': return e|g;
    case 'o': return c|d|e|g;
    case 'g': return a|b|c|d|g|f;
    default: return 0;
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

}

void loop() {

  static char buf[32];
  if (rtc_enabled) {
    static uint8_t hour, minute, sec;
    DateTime now = rtc.now();
    hour = now.hour();
    minute = now.minute();
    sec = now.second();
    snprintf(buf, 6, "%02d%c%02d", hour, (sec % 2 == 0) ? ':' : ' ', minute);
  } else {
    snprintf(buf, 6, "Pr og");
  }

  hvwrite(d1 | lookup(buf[0]));
  hvwrite(d2 | lookup(buf[1]));
  hvwrite(dd | lookup(buf[2]));
  hvwrite(d3 | lookup(buf[3]));
  hvwrite(d4 | lookup(buf[4]));

}

