#include <TinyWireM.h>
#include "segments.h"
#include "hvshift.h"

// jumper pin to enable i2c programming mode
#define JPROG 3
static bool progmode = true;

void setup() {

  pinMode(JPROG, INPUT_PULLUP);
  hv.begin();

  if (digitalRead(JPROG) != LOW) {
    TinyWireM.begin();
    progmode = false;
  }
  
}

// decode binary coded decimal number
uint8_t bcdDecode(uint8_t b) {
  return ((b / 16 * 10) + (b % 16));
}

void loop() {

  static char buf[32];
  if (!progmode) {

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
  
  } else {

    snprintf(buf, 6, "~~~~");
    if (millis() % 1000 < 50) {
      buf[4] = '.';
    }

  }

  // print buffer to display
  hv.text(buf);

}

