#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <assert.h>

#include "segments.h"
#include "i2c_peripheral.h"


// setup the main cpu clock control to run at full speed
void init_cpufrequency() {
  //! check that fuse 0x02 OSCCFG has set FREQSEL = 0x02 to run at 20MHz
  assert((0b11 & FUSE_OSCCFG) == FREQSEL_20MHZ_gc);
  assert(F_CPU == 20000000L);
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc); // select internal oscillator
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, (0 << CLKCTRL_PEN_bp));    // disable prescaler for full 20 MHz
}


// ----- filament driver -----
#define filFWD  PIN2_bm // PB2
#define filREV  PIN3_bm // PB3
#define filVREF PIN6_bm // PA6

// setup output pins to filament driver
void filament_init() {
  // setup output pins
  PORTA.DIRSET = filVREF;         // setup vref pin as output
  PORTA.OUTCLR = filVREF;         // start with reference voltage low
  PORTB.DIRSET = filFWD | filREV; // setup direction pins as outputs
  PORTB.OUTSET = filFWD;          // set one direction pin high
  // setup DAC on vref pin
  DAC0.DATA = 0;
  register8_t vref = (VREF.CTRLA & ~(0b00000111)); // read & mask the current ctrla setting
  VREF.CTRLA = vref | VREF_DAC0REFSEL_2V5_gc;      // select internal 2.5 V voltage reference for dac
  DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;       // enable dac and output to PA6
}

// write value to filament Vref DAC pin
void filament_vref(uint8_t level) {
  DAC0.DATA = level;
}

// switch filament drive direction
void filament_dir() {
  PORTB.OUTTGL = filFWD | filREV;
}


// ----- hv5812 shift register for anodes -----
#define hvDATA   PIN1_bm // PA1
#define hvCLOCK  PIN3_bm // PA3
#define hvSTROBE PIN4_bm // PA4

// setup spi controller to communicate with hv5812 shift register
void anodes_init() {
  PORTA.DIRSET = hvDATA | hvCLOCK | hvSTROBE;  // setup pins to hv5812 as outputs
  PORTA.OUTCLR = hvDATA | hvCLOCK | hvSTROBE;  // set all pins low explicitly
  SPI0.CTRLA  = SPI_PRESC_DIV4_gc;             // set spi prescaler to result in ca. 5 MHz clock
  SPI0.CTRLA |= SPI_MASTER_bm | SPI_ENABLE_bm; // enable spi in controller mode
  SPI0.CTRLB  = SPI_BUFEN_bm | SPI_SSD_bm;     // double-buffered mode, don't use chip select
}

// send data to the shift register and latch to vfd outputs
void anodes_display(uint16_t data) {
  PORTA.OUTCLR = hvSTROBE;
  while (!(SPI0.INTFLAGS & SPI_DREIF_bm)) { ; }
  SPI0.DATA = 0xFF & (data >> 8);
  while (!(SPI0.INTFLAGS & SPI_DREIF_bm)) { ; }
  SPI0.DATA = 0xFF & (data);
  while (!(SPI0.INTFLAGS & SPI_TXCIF_bm)) { ; }
  SPI0.INTFLAGS |= SPI_TXCIF_bm;
  PORTA.OUTSET = hvSTROBE;
  asm volatile("nop");
  PORTA.OUTCLR = hvSTROBE;
}


// ----- prepared display contents -----
uint16_t digits[] = { 0, 0, 0, 0, 0 };
volatile int digit = 0;

// double-lookup for brightness corrections
const uint8_t digitloop[] = { 0, 1, 2, 3, 4 };
#define DIGITS (sizeof(digitloop)/sizeof(uint8_t))

// setup the periodic interrupt timer for digit time-multiplexing
void digit_multiplexing_init() {
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;                 // use internal 32kHz
  RTC.PITINTCTRL = RTC_PI_bm;                        // enable periodic interrupt
  // RTC.PITCTRLA = RTC_PERIOD_CYC32_gc | RTC_PITEN_bm; // enable PIT with 1 msec period
  RTC.PITCTRLA = RTC_PERIOD_CYC64_gc | RTC_PITEN_bm; // enable PIT with 1 msec period
  sei();
}

// periodic interrupt service routing for digit time-multiplexing
ISR(RTC_PIT_vect) {
  RTC.PITINTFLAGS = RTC_PI_bm;    // clear interrupt flag
  const uint8_t d = digitloop[digit]; // current digit in loop
  anodes_display(grids[d] | digits[d]); // display next digit
  digit = (digit+1) % DIGITS;     // increment digit position
  if (digit == 0) filament_dir(); // toggle filament drive direction each pass
}


// ----- helper functions -----

// light up random segments
void noise() {
  digits[0] = (SEGMENTMASK & rand());
  digits[1] = (SEGMENTMASK & rand());
  digits[2] = 0;
  digits[3] = (SEGMENTMASK & rand());
  digits[4] = (SEGMENTMASK & rand());
}

// map a string buffer to segments
void text(volatile const char* buf) {
  digits[0] = segment_lookup(buf[0]);
  digits[1] = segment_lookup(buf[1]);
  digits[2] = segment_lookup(buf[2]);
  digits[3] = segment_lookup(buf[3]);
  digits[4] = segment_lookup(buf[4]);
}

// cycle and print vref value for filament brightness
void brightnessloop() {
  char buf[10];
  for (uint8_t i = 55; i < 255; i++) {
    sprintf(buf, "%04d ", i);
    text(buf);
    filament_vref(i);
    if (i  <  70) _delay_ms(750);
    if (i  < 120) _delay_ms(100);
    if (i  < 255) _delay_ms( 10);
    if (i == 254) _delay_ms(300);
  }
}


/* i2c wishlist:
 * (https://github.com/sparkfun/Serial7SegmentDisplay/wiki/Special-Commands#decimal)
 *   colon on/off (cursor control)
 *   begin every write left and then loop aroung with every fourth byte
 *   brightness control
 *   set specific digit
 *   raw data entry escape
 *   address config
 *   factory reset ?!
 *   store configuration in eeprom
 */

int main(void) {

  // setup essentials
  init_cpufrequency();
  filament_init();
  filament_vref(160);
  anodes_init();
  anodes_display(0x0000);
  digit_multiplexing_init();

  // --> show fixed string
  // text("88:88");

  // --> display white noise
  // while (1) { noise(50); _delay_ms(30); }
  
  // --> cycle and print filament brightness
  // while (1) brightnessloop();

  // --> run as an i2c peripheral
  const uint8_t address = 0x68;
  i2c_peripheral_init(address);

  // loop forever, so chip doesn't halt
  while (1);

}
