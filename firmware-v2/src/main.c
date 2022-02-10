#ifndef __AVR_ATtiny414__
#define __AVR_ATtiny414__
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <assert.h>

#include "segments.h"

// filament pins
#define filFWD  PIN2_bm // PB2
#define filREV  PIN3_bm // PB3
#define filVREF PIN6_bm // PA6

// hv5812 pins
#define hvDATA   PIN1_bm // PA1
#define hvCLOCK  PIN3_bm // PA3
#define hvSTROBE PIN4_bm // PA4

// setup the main cpu clock control to run at full speed
void init_cpufrequency() {
  //! check that fuse 0x02 OSCCFG has set FREQSEL = 0x02 to run at 20MHz
  assert((0b11 & FUSE_OSCCFG) == FREQSEL_20MHZ_gc);
  assert(F_CPU == 20000000L);
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc); // select internal oscillator
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, (0 << CLKCTRL_PEN_bp));    // disable prescaler for full 20 MHz
}

// setup output pins to filament driver
void init_filament(uint8_t dac_data) {
  // setup output pins
  PORTA.DIRSET = filVREF;         // setup vref pin as output
  PORTA.OUTCLR = filVREF;         // start with reference voltage low
  PORTB.DIRSET = filFWD | filREV; // setup direction pins as outputs
  PORTB.OUTSET = filFWD;          // set one direction pin high
  // setup dac for vref pin
  DAC0.DATA  = dac_data;                     // write specified value
  VREF.CTRLA = VREF_DAC0REFSEL_2V5_gc;       // select internal 2.5 V voltage reference
  DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm; // enable dac and output to PA6
}

// send data to the shift register and latch to vfd outputs
void display(uint16_t data) {
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

// setup spi controller to communicate with hv5812 shift register
void init_hv5812() {
  PORTA.DIRSET = hvDATA | hvCLOCK | hvSTROBE;  // setup pins to hv5812 as outputs
  PORTA.OUTCLR = hvDATA | hvCLOCK | hvSTROBE;  // set all pins low explicitly
  SPI0.CTRLA  = SPI_PRESC_DIV4_gc;             // set spi prescaler to result in ca. 5 MHz clock
  SPI0.CTRLA |= SPI_MASTER_bm | SPI_ENABLE_bm; // enable spi in controller mode
  SPI0.CTRLB  = SPI_BUFEN_bm | SPI_SSD_bm;     // double-buffered mode, don't use chip select
  display(0);                                  // all segments and grids off initially
}


volatile int digit = 0;
uint16_t digits[] = {
  G1 | Aa|Ad|Ae|Af|Ag, // E
  G2 | Ab|Ac|Ae|Af|Ag, // H
  G4 | Ad|Ae|Af, // L
  G5 | Aa|Ab|Ac|Ad|Ae|Af, // O
};
#define DIGITS (sizeof(digits)/sizeof(uint16_t))

// periodic interrupt service routing for digit time-multiplexing
ISR(RTC_PIT_vect) {
  RTC.PITINTFLAGS = RTC_PI_bm;    // clear interrupt flag
  display(digits[digit]);         // display next digit
  digit = (digit+1) % DIGITS;     // increment digit position
  PORTB.OUTTGL = filFWD | filREV; // toggle filament drive direction
}

// setup the periodic interrupt timer for digit time-multiplexing
void init_digit_multiplexing() {
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;                 // use internal 32kHz
  RTC.PITINTCTRL = RTC_PI_bm;                        // enable periodic interrupt
  RTC.PITCTRLA = RTC_PERIOD_CYC32_gc | RTC_PITEN_bm; // enable PIT with 1 msec period
  sei();
}

void main(void) {

  init_cpufrequency();
  init_filament(160);
  init_hv5812();
  init_digit_multiplexing();

  // show EHLO at beginning
  digits[0] = G1 | segment_lookup('E');
  digits[1] = G2 | segment_lookup('H');
  digits[2] = G4 | segment_lookup('L');
  digits[3] = G5 | segment_lookup('O');
  _delay_ms(1500);

  // infinite loop with random noise
  const int delay = 30;
  while (1) {
    digits[0] = G1 | (SEGMENTMASK & rand());
    digits[1] = G2 | (SEGMENTMASK & rand());
    digits[2] = G4 | (SEGMENTMASK & rand());
    digits[3] = G5 | (SEGMENTMASK & rand());
    _delay_ms(delay);
  }

}
