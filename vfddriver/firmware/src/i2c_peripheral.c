#include <avr/io.h>
#include <avr/interrupt.h>

#include "i2c_peripheral.h"

// external raw data for display
extern volatile uint16_t digits[5];
extern uint16_t segment_lookup(char ch);
extern uint16_t segment_bitmask(uint8_t input);
// set the display brightness
extern void filament_vref(uint8_t level);

// data bytes sent by peripheral or received from controller
volatile uint8_t i2c_buffer[5] = { 0 };

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

// different modes / command bytes
#define cmdBrightness  0x10
#define cmdClear       0x7f
#define cmdRaw         0xfb
#define cmdText        0xfc


// aliases for readability
inline void nack() { TWI0.SCTRLB = TWI_ACKACT_NACK_gc | TWI_SCMD_COMPTRANS_gc; }
inline void  ack() { TWI0.SCTRLB = TWI_ACKACT_ACK_gc  | TWI_SCMD_RESPONSE_gc;  }

// interrupt routine
ISR(TWI0_TWIS_vect) {

  static uint8_t command;
  static uint8_t buf_index;
  const uint8_t status = TWI0.SSTATUS;

  // abort on some error states
  if ((status & TWI_COLL_bm) // collision
   || (status & TWI_BUSERR_bm) // bus error
   || ((status & TWI_APIF_bm) && (status & TWI_DIF_bm))) { // illegal state
    return nack();
  }

  // stop condition
  if ((status & TWI_APIF_bm) && (!(status & TWI_AP_bm))) {
    command = 0;
    buf_index = 0;
    return nack();
  }

  // address detection
  if ((status & TWI_APIF_bm) && (status & TWI_AP_bm)) {
    command = 0;
    buf_index = 0; // reset counter
    return ack();
  }

  // data interrupt, action needed
  if (status & TWI_DIF_bm) {

    // controller wants to read
    if (status & TWI_DIR_bm) {
      // always return all-zero
      TWI0.SDATA = 0x00;
      ack();
    }

    // controller wants to write
    else {
      const uint8_t data = TWI0.SDATA; 

      // first byte could be a command
      if (command == 0) {
        buf_index = 0;
        // set brightness
        if (data == cmdBrightness) {
          command = cmdBrightness;
          ack();
          return;
        }
        // set raw segments
        if (data == cmdRaw) {
          command = cmdRaw;
          ack();
          return;
        }
        // intepret following bytes as text
        if (data == cmdText) {
          command = cmdText;
          ack();
          return;
        }
        // clear display contents
        if (data == cmdClear) {
          command = cmdClear;
          for (uint8_t i = 0; i < 5; i++) {
            digits[i] = 0x0000;
          };
          ack();
          return;          
        }
        // otherwise, try to interpret this byte as text
        command = cmdText;
      }

      // set brightness on second byte
      if (command == cmdBrightness) {
        if (buf_index < 1) {
          filament_vref(data);
          buf_index++;
        }
      };

      // set raw segments from bytes
      if (command == cmdRaw) {
        if (buf_index < 5) {
          digits[buf_index++] = segment_bitmask(data);
        };
      };

      // interpret as text
      if (command == cmdText) {
        if (buf_index < 5) {
          digits[buf_index++] = segment_lookup(data);
        };
      };

    }
    ack();
    return;
  }

  // if we've gotten here there was likely an error
  return nack();

}
