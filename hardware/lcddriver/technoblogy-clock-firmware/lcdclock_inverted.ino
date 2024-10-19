/* Low-Power LCD Clock

   David Johnson-Davies - www.technoblogy.com - 4th May 2021
   AVR128DA48 @ 24 MHz (internal oscillator; BOD disabled)
   
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// Seven segment definitions

const int CharLen = 15;
uint8_t Char[CharLen] = {
//   _a     //   _d  (mirrored)
// f| |b    // c| |e
//   _g     //   _g
// e| |c    // b| |f
//   -d     //   -a
//  abcdefg  Segments
  0b1111110, // 0
  0b0000110, // 1
  0b1101101, // 2
  0b1001111, // 3
  0b0010111, // 4
  0b1011011, // 5
  0b1111011, // 6
  0b0001110, // 7
  0b1111111, // 8
  0b1011111, // 9
  0b0000000, // 10  Space
  0b0000001, // 11  '-'
  0b0011101, // 12  Degree
  0b1111000, // 13  'C'
  0b1110110  // 14  'V'
};

const int Space = 10;
const int Minus = 11;
const int Degree = 12;
const int Celsius = 13;
const int Vee = 14;

// Ports **********************************************

PORT_t *Digit[4] = { &PORTB, &PORTA, &PORTC, &PORTD };

void PortSetup () {
  for (int p=0; p<4; p++) Digit[p]->DIR = 0xFF;       // All pins outputs
  PORTE.DIR = PIN0_bm;                      // COMs outputs, PE0 and PE1
  PORTF.DIR = PIN5_bm | PIN4_bm;                      // 1A, colon
}

// Real-Time Clock **********************************************

void RTCSetup () {
  uint8_t temp;
  // Initialize 32.768kHz Oscillator:

  // Disable oscillator:
  temp = CLKCTRL.XOSC32KCTRLA & ~CLKCTRL_ENABLE_bm;

  // Enable writing to protected register
  CPU_CCP = CCP_IOREG_gc;
  CLKCTRL.XOSC32KCTRLA = temp;

  while (CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm);   // Wait until XOSC32KS is 0
  
  temp = CLKCTRL.XOSC32KCTRLA | CLKCTRL_LPMODE_bm;      // Use External Crystal & low power
  
  // Enable writing to protected register
  CPU_CCP = CCP_IOREG_gc;
  CLKCTRL.XOSC32KCTRLA = temp;
  
  temp = CLKCTRL.XOSC32KCTRLA | CLKCTRL_ENABLE_bm;    // Enable oscillator
  
  // Enable writing to protected register
  CPU_CCP = CCP_IOREG_gc;
  CLKCTRL.XOSC32KCTRLA = temp;
  
  // Initialize RTC
  while (RTC.STATUS > 0);                             // Wait until registers synchronized

  // 32.768kHz External Crystal Oscillator (XOSC32K)
  RTC.CLKSEL = RTC_CLKSEL_XOSC32K_gc;

  RTC.PITINTCTRL = RTC_PI_bm;                         // Periodic Interrupt: enabled
  
  // RTC Clock Cycles 512, enabled ie 64Hz interrupt
  RTC.PITCTRLA = RTC_PERIOD_CYC512_gc | RTC_PITEN_bm;
}

// Display Time **********************************************

void DisplayTime (unsigned long halfsecs) {
  uint8_t minutes = (halfsecs / 120) % 60;
  #ifdef TWELVEHOUR
  uint8_t hours = (halfsecs / 7200) % 12 + 1;
  #else
  uint8_t hours = (halfsecs / 7200) % 24;
  #endif
  uint8_t hourdec = Char[hours/10];
  Digit[0]->OUT = hourdec;
  Digit[1]->OUT = Char[hours%10];
  Digit[2]->OUT = Char[minutes/10];
  Digit[3]->OUT = Char[minutes%10];
  uint8_t colon = ((halfsecs >> 1) & 1)<<4;                  // Toggle colon at 1Hz   
  PORTF.OUT = (hourdec>>1 & PIN5_bm) | colon;
}

// Interrupt Service Routine at 64Hz
ISR(RTC_PIT_vect) {
  static uint8_t cycles = 0;
  static unsigned long halfsecs;
  RTC.PITINTFLAGS = RTC_PI_bm;                        // Clear interrupt flag
  // Toggle segments
  for (int p=0; p<4; p++) Digit[p]->OUTTGL = 0xFF;    // Toggle all PORTA,B,C,D pins
  PORTE.OUTTGL = PIN0_bm;                   // Toggle COMs, PE0 and PE1
  PORTF.OUTTGL = PIN5_bm | PIN4_bm;                   // Toggle segment 1A, Colon

  cycles++;
  if (cycles < 32) return;
  cycles = 0;

  // Update time
  halfsecs = (halfsecs+1) % 172800;                   // 24 hours
                 
  if (MinsButton()) halfsecs = ((halfsecs/7200)*60 + (halfsecs/120 + 1)%60)*120;
  if (HoursButton()) halfsecs = halfsecs + 7200;

  DisplayTime(halfsecs);
}

// Buttons **********************************************

void ButtonSetup () {
  PORTE.PIN3CTRL = PORT_PULLUPEN_bm;                  // PE3 input pullup
  PORTE.PIN2CTRL = PORT_PULLUPEN_bm;                  // PE2 input pullup
}

boolean MinsButton () {
  return (PORTE.IN & PIN2_bm) == 0;                   // True if button pressed
}

boolean HoursButton () {
  return (PORTE.IN & PIN3_bm) == 0;                   // True if button pressed
}

// Sleep **********************************************

void SleepSetup () {
  SLPCTRL.CTRLA |= SLPCTRL_SMODE_PDOWN_gc;
  SLPCTRL.CTRLA |= SLPCTRL_SEN_bm;
}

// Setup **********************************************

void setup () {
  PortSetup();
  ButtonSetup();
  SleepSetup();
  RTCSetup();
}

// Just stay asleep to save power unless woken by an interrupt
void loop () {
  sleep_cpu();
}
