#ifndef __SEGMENTS_H_
#define __SEGMENTS_H_

// pinout on ivl2-5/7, bottom row:
//   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
//  Fil G1  Adt Ag  Ae  G2  Ac  Gd  Adb Ad  G3  Ab  Af  Aa  G4  Fil

// anode grids / digits
//  8   8   :   8   8
//  G1  G2  Gd  G3  G4
#define G1 (1 <<  0)
#define G2 (1 << 13)
#define Gd (1 << 11)
#define G3 (1 <<  8)
#define G4 (1 <<  4)

// anode segments
//       a
//     ────
//  f │    │
//    │  g │ b   ■ dt
//     ────
//  e │    │
//    │  d │ c   ■ db
//     ────
#define Aa  (1 <<  5)
#define Ab  (1 <<  7)
#define Ac  (1 << 12)
#define Ad  (1 <<  9)
#define Ae  (1 <<  3)
#define Af  (1 <<  6)
#define Ag  (1 <<  2)
#define Adt (1 <<  1)
#define Adb (1 << 10)
#define SEGMENTS (Aa|Ab|Ac|Ad|Ae|Af|Ag|Adt|Adb)

// some constant numbers
#define GRIDS       5 // number of grids
#define SHIFTWIDTH 14 // number of bits to shift each time

// character lookup for segment mapping
unsigned lookup(char ch);

#endif
