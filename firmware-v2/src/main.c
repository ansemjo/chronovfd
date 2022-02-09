#ifndef __AVR_ATtiny414__
#define __AVR_ATtiny414__
#endif

// TODO: fix frequency / delays

#include <avr/io.h>
#include <util/delay.h>

#include "segments.h"

// filament pins
#define filFWD  PIN2_bm // PB2
#define filREV  PIN3_bm // PB3
#define filVREF PIN6_bm // PA6

// hv5812 pins
#define hvDATA   PIN1_bm // PA1
#define hvCLOCK  PIN3_bm // PA3
#define hvSTROBE PIN4_bm // PA4

void init_filament(uint8_t dac_data) {

  // setup filament output pins
  PORTA.DIRSET = filVREF;
  PORTB.DIRSET = filFWD | filREV;

  PORTA.OUTCLR = filVREF; // start with reference low
  PORTB.OUTSET = filFWD;  // set one direction high

  // setup DAC for VREF pin
  VREF.CTRLA = VREF_DAC0REFSEL_2V5_gc; // select internal 2.5 V reference voltage
  DAC0.DATA  = dac_data; // write value
  DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm; // enable output

}

void fildir() {
  // change filament drive direction
  PORTB.OUTTGL = filFWD | filREV;
}

void init_hv5812() {

  // setup hv5812 output pins
  PORTA.DIRSET = hvDATA | hvCLOCK | hvSTROBE;
  PORTA.OUTCLR = hvDATA | hvCLOCK | hvSTROBE;

  // configure SPI peripheral
  SPI0.CTRLA = SPI_MASTER_bm | SPI_PRESC_DIV4_gc | SPI_ENABLE_bm;
  SPI0.CTRLB = SPI_BUFEN_bm | SPI_SSD_bm | SPI_MODE_0_gc;

  // turn off everything
  SPI0.DATA = 0x00;
  while (!(SPI0.INTFLAGS & SPI_DREIF_bm)) { ; }
  SPI0.DATA = 0x00;
  while (!(SPI0.INTFLAGS & SPI_TXCIF_bm)) { ; }
  SPI0.INTFLAGS |= SPI_TXCIF_bm;
  PORTA.OUTSET = hvSTROBE;
  asm volatile("nop");
  PORTA.OUTCLR = hvSTROBE;

}

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

void main(void) {

  init_filament(100);
  init_hv5812();

  const int delay = 12;
  while (1) {
    fildir();
    display(segment_lookup('E') | G1);
    _delay_ms(delay);
    fildir();
    display(segment_lookup('H') | G2);
    _delay_ms(delay);
    fildir();
    display(segment_lookup('L') | G4);
    _delay_ms(delay);
    fildir();
    display(segment_lookup('O') | G5);
    _delay_ms(delay);
  }

}
