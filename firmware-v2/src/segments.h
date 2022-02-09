// Copyright (c) 2020 Anton Semjonov
// Licensed under the MIT License

#pragma once

#include <stdint.h>

/*
This file handles the mapping of bits in a uint16_t SPI data packet sent to the
HV5812 high-voltage display driver to the individual segments and grids of the
vacuum display.

                          VFD driver pinout to IVL2-5/7:
             |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
function:   Fil G1  Adt Ag  Ae  G2  Ac  Gd  Adb Ad  G3  Ab  Af  Aa  G4  Fil
hv5812 pin:     10   9   8   7   6   5   4   3   2   1  11  12  13  14
*/

// anode digit grids:
//   8   8   :   8   8
//  G1  G2  G3  G4  G5
#define G1 (1 << 13)
#define G2 (1 <<  0)
#define G3 (1 <<  2)
#define G4 (1 <<  5)
#define G5 (1 <<  9)
#define GRIDS 5
#define GRIDMASK (G1|G2|G3|G4|G5)

// array of grids for looping
const uint16_t grids[GRIDS];
const uint8_t gridpos[5];

// anode segments:
//       a
//     ────
//  f │    │
//    │  g │ b   ■ dt
//     ────
//  e │    │
//    │  d │ c   ■ db
//     ────
#define Aa  (1 <<  8)
#define Ab  (1 <<  6)
#define Ac  (1 <<  1)
#define Ad  (1 <<  4)
#define Ae  (1 << 10)
#define Af  (1 <<  7)
#define Ag  (1 << 11)
#define Adt (1 << 12)
#define Adb (1 <<  3)
#define SEGMENTMASK (Aa|Ab|Ac|Ad|Ae|Af|Ag|Adt|Adb)

// character lookup for segment mapping
uint16_t segment_lookup(char ch);
