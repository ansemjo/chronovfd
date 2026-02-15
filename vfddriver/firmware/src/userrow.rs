use byteorder::ByteOrder;

use crate::charmap::character;
use heapless::Vec;

pub struct UserSettings {
    pub i2c_address: u8,
    pub filament_brightness: u8,
    pub digit_multiplex_frequency: u16,
    pub initial_digits: [u16; 5],
    pub grid_loop: Vec<u8, 16>,
}

// Read user settings from USERROW EEPROM (0x1300); 0xff in USERROW means "use default"
// - 0x00 -> I2C peripheral address: u8
// - 0x01 -> initial filament brightness DAC: u8
// - 0x02 -> digit multiplexing frequency: u16 (big-endian)
// - 0x0b -> initial digits to display: [u8; 5] (mapped with fn character)
// - 0x10 -> grid loop for brightness equalization: [u8; 16]
//     i.e. 0x00 0x00 0x01 0x02 0x03 0x04 0xff ... --> [ 0, 0, 1, 2, 3, 4 ]
pub fn read_settings(usr: &avr_device::attiny414::USERROW) -> UserSettings {
    let readbyte = |n: usize| usr.userrow(n).read().bits();

    // read peripheral address or use 0x42 by default
    let mut addr = readbyte(0x00);
    if addr == 0xff { addr = 0x42 };

    // initial value for the filament driver Vref
    let mut filament = readbyte(0x01);
    if filament == 0xff { filament = 120 };

    // multiplexing frequency in Hz
    let mut freq = byteorder::BigEndian::read_u16(&[readbyte(0x02), readbyte(0x03)]);
    if freq == 0xffff { freq = 10_000 };

    // read initial display contents; default 0xff in eeprom will just light up all segments
    let mut segments = [0xffff; 5];
    for i in 0..5 { segments[i] = character(readbyte(0x0b + i)); }

    // read configured anode grid loop from eeprom; first byte decides default
    let mut grids = Vec::<u8, 16>::new();
    if readbyte(0x10) == 0xff {
        // each grid once, left to right
        for i in 0..5 { grids.push(i).unwrap(); }
    } else {
        for i in 0..16 {
            let grid = readbyte(0x10 + i);
            if grid == 0xff { break };
            grids.push(grid).unwrap();
        }
    }


    UserSettings {
        i2c_address: addr,
        filament_brightness: filament,
        digit_multiplex_frequency: freq,
        initial_digits: segments,
        grid_loop: grids,
    }

}