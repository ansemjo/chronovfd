#![allow(non_upper_case_globals,dead_code)]

/*
This file handles the mapping of bits in a uint16_t SPI data packet sent to the
HV5812 high-voltage display driver to the individual segments and grids of the
vacuum display.

               --      --             --      -- 
              |  |    |  |     O     |  |    |  |
               --      --             --      -- 
              |  |    |  |     O     |  |    |  |
               --      --             --      -- 

                 VFD driver pinout to IVL2-5/7:

  Fil G1  Adt Ag  Ae  G2  Ac  Gc  Adb Ad  G3  Ab  Af  Aa  G4  Fil
   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
   x  13  12  11  10   0   1   2   3   4   5   6   7   8   9   x  (bit)

*/

// anode digit grids:
//   8   8   :   8   8
//  G1  G2  Gc  G3  G4
pub const G1: u16 = 1 << 13;
pub const G2: u16 = 1 <<  0;
pub const Gc: u16 = 1 <<  2;
pub const G3: u16 = 1 <<  5;
pub const G4: u16 = 1 <<  9;

pub const GRIDS: [u16; 5] = [ G1, G2, Gc, G3, G4 ];

// anode digit segments:
//       a
//     ────
//  f │    │
//    │  g │ b   ■ dt
//     ────
//  e │    │
//    │  d │ c   ■ db
//     ────
pub const Aa:  u16 = 1 <<  8;
pub const Ab:  u16 = 1 <<  6;
pub const Ac:  u16 = 1 <<  1;
pub const Ad:  u16 = 1 <<  4;
pub const Ae:  u16 = 1 << 10;
pub const Af:  u16 = 1 <<  7;
pub const Ag:  u16 = 1 << 11;
pub const Adt: u16 = 1 << 12;
pub const Adb: u16 = 1 <<  3;

/// bitmask with all segments to filter out any grids by bitwise-and
pub const SEGMENT_MASK: u16 = Aa|Ab|Ac|Ad|Ae|Af|Ag|Adt|Adb;

// map a raw byte bitmask to segment anodes
pub fn sevensegment(input: u8) -> u16 {
  let mut out = 0;
  if (input & 0b0100_0000) != 0 { out |= Aa };
  if (input & 0b0010_0000) != 0 { out |= Ab|Adt };
  if (input & 0b0001_0000) != 0 { out |= Ac|Adb };
  if (input & 0b0000_1000) != 0 { out |= Ad };
  if (input & 0b0000_0100) != 0 { out |= Ae };
  if (input & 0b0000_0010) != 0 { out |= Af };
  if (input & 0b0000_0001) != 0 { out |= Ag };
  return out;
}