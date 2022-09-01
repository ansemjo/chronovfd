#include <avr/io.h>
#include <avr/interrupt.h>

#include "i2c_peripheral.h"

extern volatile uint8_t i2c_data[]; // data byte sent by peripheral or received from controller

// Setup the I2C peripheral and enable interrupt.
// If needed, configure `TWI0.CTRLA` yourself before.
void i2c_peripheral_init(uint8_t address) {
  PORTA.DIRSET = PIN1_bm | PIN2_bm; // make pins outputs, just in case
  TWI0.SADDR = address << 1;        // set our peripheral address
  TWI0.SADDRMASK = 0;               // disable second address mask
  TWI0.SCTRLA = 1 << TWI_DIEN_bp    // data interrupt enable
              | 1 << TWI_APIEN_bp   // address or stop interrupt enable
              | 1 << TWI_PIEN_bp    // stop interrupt enable
              | 1 << TWI_ENABLE_bp; // enable TWI peripheral
}

// aliases for readability
inline void nack() { TWI0.SCTRLB = TWI_ACKACT_NACK_gc | TWI_SCMD_COMPTRANS_gc; }
inline void  ack() { TWI0.SCTRLB = TWI_ACKACT_ACK_gc  | TWI_SCMD_RESPONSE_gc;  }

// interrupt routine
ISR(TWI0_TWIS_vect) {

  static unsigned long num_bytes;
  const uint8_t status = TWI0.SSTATUS;

  // abort on some error states
  if ((status & TWI_COLL_bm) // collision
   || (status & TWI_BUSERR_bm) // bus error
   || ((status & TWI_APIF_bm) && (status & TWI_DIF_bm))) { // illegal state
    return nack();
  }

  // stop condition
  if ((status & TWI_APIF_bm) && (!(status & TWI_AP_bm))) {
    return nack();
  }

  // address detection
  if ((status & TWI_APIF_bm) && (status & TWI_AP_bm)) {
    num_bytes = 0; // reset counter
    return ack();
  }

  // data interrupt, action needed
  if (status & TWI_DIF_bm) {
    if (status & TWI_DIR_bm) {
      // controller wants to read
      // TODO: handle missing RXACK
      if (num_bytes < 4) {
        TWI0.SDATA = i2c_data[num_bytes];
      } else {
        TWI0.SDATA = 0xff;
      }
      ack();
    } else {
      // controller wants to write
      if (!(num_bytes < 4)) {
        return TWI0.SCTRLB = TWI_ACKACT_NACK_gc | TWI_SCMD_COMPTRANS_gc; // nack
      }
      i2c_data[num_bytes] = TWI0.SDATA;
      ack();
    }

    if (num_bytes < 4) num_bytes++;
    return;
  }

  // if we've gotten here there was likely an error
  return nack();

}
