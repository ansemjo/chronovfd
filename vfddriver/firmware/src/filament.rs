use core::mem;
use avr_device::attiny414;

static mut RTC: mem::MaybeUninit<attiny414::RTC> = mem::MaybeUninit::uninit();
static mut PORTB: mem::MaybeUninit<attiny414::PORTB> = mem::MaybeUninit::uninit();

// configure the filament supply using a DAC to H-bridge Vref
pub fn setup_driver(port_a: &attiny414::PORTA, port_b: &attiny414::PORTB, vref: &attiny414::VREF, dac: &attiny414::DAC0, initial_brightness: u8) {

    // configure pins for the filament driver
    // fwd: PB2, rev: PB3, vref: PA6
    port_b.dirset().write(|w| w.pb2().set_bit().pb3().set_bit());
    port_a.dirset().write(|w| w.pa6().set_bit()); // vref

    // configure DAC0 on vref pin
    vref.ctrla().write(|w| w.dac0refsel()._2v5()); // use internal 2.5V reference
    dac.ctrla().write(|w| w.enable().set_bit().outen().set_bit()); // enable output
    brightness(&dac, initial_brightness);

    // set one direction pin high to enable filament current
    port_b.outset().write(|w| w.pb2().set_bit()); // forward

}

// set filament brightness by adjusting Vref DAC
// value: barely on (60) .. filament starts glowing (180)
pub fn brightness(dac: &attiny414::DAC0, value: u8) {
    dac.data().write(|w| w.set(value));
}

// configure RTC periodic interrupt to toggle filament drive direction
pub fn setup_toggle_isr(rtc: attiny414::RTC, portb: attiny414::PORTB) {

    // configure RTC periodic interrupt at 1 kHz
    rtc.clksel().write(|w| w.clksel().int32k());
    rtc.pitintctrl().write(|w| w.pi().set_bit());
    rtc.pitctrla().write(|w| w.period().cyc32().piten().set_bit());

    // move needed peripherals to static for ISR
    unsafe {
        RTC   = mem::MaybeUninit::new(rtc);
        PORTB = mem::MaybeUninit::new(portb);
    }

}

#[avr_device::interrupt(attiny414)]
fn RTC_PIT() {
    
    // clear RTC interrupt flag
    #[allow(static_mut_refs)]
    let rtc = unsafe { RTC.assume_init_ref() };
    rtc.pitintflags().write(|w| w.pi().set_bit());

    // toggle filament drive direction
    #[allow(static_mut_refs)]
    let port_b = unsafe { PORTB.assume_init_ref() };
    port_b.outtgl().write(|w| w.pb2().set_bit().pb3().set_bit());

}