#![no_std]
#![no_main]
#![feature(abi_avr_interrupt)]

mod anodes;
mod charmap;
mod filament;
mod i2c;
mod segments;
mod userrow;

use crate::segments::*;

use panic_halt as _;
use avr_device::{asm, attiny414, interrupt::{self, Mutex}};
use core::{cell::Cell};

// this is the display data
static DIGITS: Mutex<Cell<[u16; GRIDS.len()]>> = Mutex::new(Cell::new([0, 0, 0, 0, 0]));

// TODO:
// - do I need the Mutex above? can't I replace with with static mutable, if I use the same interrupt::free "fences"?
// - fix DAC0 steal() in i2c.rs, without having to introduce unsafe in brightness()?

#[avr_device::entry]
fn main() -> ! {

    // take peripherals
    let avr = attiny414::Peripherals::take().unwrap();

    // read settings from USERROW EEPROM
    let settings = userrow::read_settings(&avr.USERROW);

    // configure peripherals
    setup_cpufreq_10mhz(&avr.FUSE, avr.CPU, avr.CLKCTRL);
    i2c::setup_peripheral(&avr.PORTB, avr.TWI0, settings.i2c_address);
    anodes::setup_shift_register(&avr.PORTA, &avr.SPI0);
    anodes::setup_grid_multiplexing(avr.TCB0, settings.digit_multiplex_frequency);
    filament::setup_driver(&avr.PORTA, &avr.PORTB, &avr.VREF, &avr.DAC0, settings.filament_brightness);
    filament::setup_toggle_isr(avr.RTC, avr.PORTB);

    // set initial display contents
    interrupt::free(|cs| { DIGITS.borrow(cs).set(settings.initial_digits); });
    
    // DANGER ZONE from here on out
    unsafe { interrupt::enable(); }

    // loop to multiplex digits onto the display
    loop {
        for grid in settings.grid_loop.iter() {

            // skip this grid if out-of-bounds
            if *grid as usize >= GRIDS.len() { continue; }

            // prepare data for shift register and transmit
            let current_digit: u16 = interrupt::free(|cs| {
                let d = DIGITS.borrow(cs);
                return d.get()[*grid as usize];
            });
            let data = (current_digit & SEGMENT_MASK) | GRIDS[*grid as usize];
            anodes::display(&avr.PORTA, &avr.SPI0, data);

            // go to sleep until next interrupt
            while !anodes::nextdigit() { asm::sleep() };

        }
    }

}

// setup the main clock to run at 10 MHz
fn setup_cpufreq_10mhz(fuse: &attiny414::FUSE, cpu: attiny414::CPU, clk: attiny414::CLKCTRL) {

    // assert that fuse config is as expected
    assert!(fuse.osccfg().read().freqsel().is_20mhz());

    // select internal oscillator
    cpu.ccp().write(|w| w.ccp().ioreg());
    clk.mclkctrla().write(|w| w.clksel().osc20m());

    // set /2 prescaler, 10 MHz is safe down to 2.7V
    cpu.ccp().write(|w| w.ccp().ioreg());
    clk.mclkctrlb().write(|w| w.pdiv()._2x().pen().set_bit());

    // lock further clock changes
    cpu.ccp().write(|w| w.ccp().ioreg());
    clk.mclklock().write(|w| w.locken().set_bit());

}