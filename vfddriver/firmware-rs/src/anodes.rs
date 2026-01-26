use core::{mem, sync::atomic};
use avr_device::{asm, attiny414, interrupt};

static mut TCB0: mem::MaybeUninit<attiny414::TCB0> = mem::MaybeUninit::uninit();
static NEXTDIGIT: atomic::AtomicBool = atomic::AtomicBool::new(false);

// configure the SPI to HV5812 anodes shift register
pub fn setup_shift_register(port_a: &attiny414::PORTA, spi: &attiny414::SPI0) {

    // configure pins for the shift register
    // mosi: PA1, sck: PA3, latch: PA4
    port_a.dirset().write(|w| w.pa1().set_bit().pa3().set_bit().pa4().set_bit());
    port_a.outclr().write(|w| w.pa1().set_bit().pa3().set_bit().pa4().set_bit());

    // configure serial peripheral interface as controller
    spi.ctrla().write(|w| w
        .dord().msb_first() // transmit msb first (left-to-right)
        .master().set_bit() // master operation
        // frequency doubler and prescaler for 10 / 4 x 2 = 5 MHz
        .clk2x().set_bit().presc().clk_per_4_2()
    );
    spi.ctrlb().write(|w| w
        .bufen().set_bit() // buffered operation
        .ssd().set_bit() // no chip select
        .mode()._0() // rising edge sample
    );

    // enable peripheral
    spi.ctrla().modify(|_, w| w.enable().set_bit());

    // clear contents initially
    display(port_a, spi, 0x0000);

}

// write something to the anode shift register
pub fn display(port_a: &attiny414::PORTA, spi: &attiny414::SPI0, data: u16) {

    // latch down and check if we are clear to send
    port_a.outclr().write(|w| w.pa4().set_bit());
    while spi.intflags().read().buffered_dreif().bit_is_clear() { };

    // write high byte and wait for data register empty (DRE) interrupt flag
    spi.data().write(|w| w.set((data >> 8) as u8));
    while spi.intflags().read().buffered_dreif().bit_is_clear() { };

    // write low byte and wait for data register empty (DRE) interrupt flag
    spi.data().write(|w| w.set((data & 0xff) as u8));
    while spi.intflags().read().buffered_dreif().bit_is_clear() { };
    
    // wait for transmission complete (TXC) and clear flag
    while spi.intflags().read().buffered_txcif().bit_is_clear() { };
    spi.intflags().write(|w| w.buffered_txcif().set_bit());

    // strobe the latch up
    port_a.outset().write(|w|w.pa4().set_bit()); // latch up
    asm::nop(); // wait one cycle
    port_a.outclr().write(|w| w.pa4().set_bit()); // latch down

}

// configure TCB0 timer interrupt for grid multiplexing
pub fn setup_grid_multiplexing(timer: attiny414::TCB0, frequency: u16) {

    // compute counter value for given frequency
    fn timervalue(freq: u16) -> u16 {
        // would overflow, thus return maximum counter value
        if freq < 153 { return 0xffff; }
        return (10_000_000 / freq as u32) as u16
    }

    // configure timer for digit multiplexing
    timer.ccmp().write(|w| w.set(timervalue(frequency)));
    timer.intctrl().write(|w| w.capt().set_bit()); // enable interrupt
    timer.ctrlb().write(|w| w.cntmode().int()); // periodic interrupt
    timer.ctrla().modify(|_, w| w.enable().set_bit()); // enable timer

    // move needed peripheral to static for ISR
    unsafe { TCB0 = mem::MaybeUninit::new(timer); }

}

#[avr_device::interrupt(attiny414)]
fn TCB0_INT() {
    
    // clear timer interrupt flag
    #[allow(static_mut_refs)]
    let tcb = unsafe { TCB0.assume_init_ref() };
    tcb.intflags().write(|w| w.capt().set_bit());

    // store bool to advance digit multiplexing
    NEXTDIGIT.store(true, atomic::Ordering::SeqCst);

}

// load the global beep atomically and reset if it was set
pub fn nextdigit() -> bool {
    return interrupt::free(|_| {
        if NEXTDIGIT.load(atomic::Ordering::SeqCst) {
            NEXTDIGIT.store(false, atomic::Ordering::SeqCst);
            return true
        } else {
            return false
        };
    });
}