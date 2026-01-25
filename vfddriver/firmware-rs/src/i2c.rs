use core::mem;
use avr_device::{attiny414, interrupt};

use crate::{charmap, filament, segments};

static mut TWI: mem::MaybeUninit<attiny414::TWI0> = mem::MaybeUninit::uninit();

// configure the SPI to HV5812 anodes shift register
pub fn setup_peripheral(port_b: &attiny414::PORTB, twi: attiny414::TWI0, address: u8) {

    // configure pins as outputs, just in case
    // scl: PB0, sda: PB1
    port_b.dirset().write(|w| w.pb0().set_bit().pb1().set_bit());

    // set our peripheral address, no secondary
    twi.saddr().write(|w| w.set(address << 1));
    twi.saddrmask().write(|w| w.addren().clear_bit());

    // enable all the interrupts and overall peripheral
    twi.sctrla().write(|w| w
        .dien().set_bit() // data interrupt
        .apien().set_bit() // address or stop
        .pien().set_bit() // stop interrupt
        .pmen().clear_bit() // only our own addr
        .smen().clear_bit() // no smart mode
        .enable().set_bit() // let's go
    );

    // move needed peripheral to static for ISR
    unsafe { TWI = mem::MaybeUninit::new(twi); }

}


#[avr_device::interrupt(attiny414)]
fn TWI0_TWIS() {

    static mut COMMAND: Command = Command::Waiting;
    static mut BUF_INDEX: usize = 0;
    
    // get reference to the peripheral
    #[allow(static_mut_refs)]
    let twi = unsafe { TWI.assume_init_ref() };

    // read status register
    let s = twi.sstatus().read();

    // shorthands to continue operation
    let nack = || { twi.sctrlb().write(|w| w.ackact().set_bit().scmd().comptrans()); return }; // err
    let  ack = || { twi.sctrlb().write(|w| w.ackact().clear_bit().scmd().response()); return }; // ok, continue

    // abort directly on some error states
    if s.coll().bit_is_set() // collision
    || s.buserr().bit_is_set() // bus error
    || (s.apif().bit_is_set() && s.dif().bit_is_set()) { // illegal state
        return nack();
    }

    // address or stop condition received
    if s.apif().bit_is_set() {
        *COMMAND = Command::Waiting;
        *BUF_INDEX = 0;
        if s.ap().bit_is_clear() { return nack(); } // stop
        if s.ap().bit_is_set() { return ack(); } // continue
    }

    // data interrupt, action needed
    if s.dif().bit_is_set() {

        // controller wants to read
        if s.dir().bit_is_set() {
            // TODO: returning dummy data for now
            twi.sdata().write(|w| w.set(0x00));
            return ack();
        }

        // we're receiving data
        if s.dir().bit_is_clear() {
            let data = twi.sdata().read().bits();

            // first byte can be a command
            if *COMMAND == Command::Waiting {

                // want to set brightness
                if data == CMD_SET_BRIGHTNESS {
                    *COMMAND = Command::SetBrightness;
                    return ack();
                }

                // clear display
                if data == CMD_CLEAR_DISPLAY {
                    *COMMAND = Command::ClearDisplay;
                    interrupt::free(|cs| { crate::DIGITS.borrow(cs).set([ 0, 0, 0, 0, 0 ]); });
                    return ack();
                }

                // otherwise we're receiving text or raw segments
                *COMMAND = Command::Text;

            }

            // set brightness
            if *COMMAND == Command::SetBrightness && *BUF_INDEX == 0 {
                unsafe {
                    // TODO: dangerous hack but should be fine inside an ISR
                    let dac = attiny414::DAC0::steal();
                    filament::brightness(&dac, data);
                }
                *BUF_INDEX = *BUF_INDEX+1;
                return ack();
            }

            // write text
            if *COMMAND == Command::Text && *BUF_INDEX < segments::GRIDS.len() {
                interrupt::free(|cs| {
                    let digits = crate::DIGITS.borrow(cs).as_array_of_cells();
                    digits[*BUF_INDEX].set(charmap::character(data));
                });
                *BUF_INDEX = *BUF_INDEX+1;
                return ack();
            }

        }

    }

    // if we ever get here, there was likely an error
    return nack();


}

// detect command bytes in i2c transmission
const CMD_SET_BRIGHTNESS: u8 = 0x10;
const CMD_CLEAR_DISPLAY: u8  = 0x7f;

#[derive(PartialEq)]
enum Command {
    Waiting,
    Text,
    SetBrightness,
    ClearDisplay,
}