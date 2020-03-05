#pragma once

// pinout on ivl2-5/7, bottom row:
//   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
//  Fil G1  Adt Ag  Ae  G2  Ac  Gd  Adb Ad  G3  Ab  Af  Aa  G4  Fil

// anode grids / digits
//  8   8   :   8   8
//  G1  G2  Gd  G3  G4
#define G1 (1 <<  3)
#define G2 (1 <<  6)
#define Gd (1 <<  8)
#define G3 (1 << 11)
#define G4 (1 <<  0)

// anode segments
//       a
//     ────
//  f │    │
//    │  g │ b   ■ dt
//     ────
//  e │    │
//    │  d │ c   ■ db
//     ────
#define Aa  (1 <<  1)
#define Ab  (1 << 12)
#define Ac  (1 <<  7)
#define Ad  (1 << 10)
#define Ae  (1 <<  5)
#define Af  (1 << 13)
#define Ag  (1 <<  4)
#define Adt (1 <<  2)
#define Adb (1 <<  9)

// some constant numbers
#define grids     5 // number of grids
#define bitwidth 14 // number of bits to shift each time

// character lookup for segment mapping
unsigned lookup(char ch) {
  switch (ch) {
    // numbers
    case '0': return Aa|Ab|Ac|Ad|Ae|Af;
    case '1': return Ab|Ac;
    case '2': return Aa|Ab|Ag|Ae|Ad;
    case '3': return Aa|Ab|Ac|Ad|Ag;
    case '4': return Af|Ag|Ab|Ac;
    case '5': return Aa|Af|Ag|Ac|Ad;
    case '6': return Aa|Af|Ae|Ad|Ac|Ag;
    case '7': return Aa|Ab|Ac;
    case '8': return Aa|Ab|Ac|Ad|Ae|Af|Ag;
    case '9': return Aa|Ab|Ac|Ad|Ag|Af;
    // alphabet
    case 'A': return Aa|Ab|Ac|Ae|Af|Ag;
    case 'a': return lookup('a');
    case 'B': return lookup('b');
    case 'b': return Ac|Ad|Ae|Af|Ag;
    case 'C': return Aa|Ad|Ae|Af;
    case 'c': return Ad|Ae|Ag;
    case 'D': return lookup('d');
    case 'd': return Ab|Ac|Ad|Ae|Ag;
    case 'E': return Aa|Ad|Ae|Af|Ag;
    case 'e': return lookup('E');
    case 'F': return Aa|Ae|Af|Ag;
    case 'f': return lookup('F');
    case 'G': return lookup('6');
    case 'g': return lookup('9');
    case 'H': return Ab|Ac|Ae|Af|Ag;
    case 'h': return Ac|Ae|Af|Ag;
    case 'I': return Ae|Af;
    case 'i': return Ae;
    case 'J': return Ab|Ac|Ad;
    case 'j': return lookup('J');
    case 'K': return lookup('k');
    case 'k': return Aa|Ac|Ae|Af|Ag;
    case 'L': return Ad|Ae|Af;
    case 'l': return lookup('I'); //           _  _
    case 'M': return Aa|Ab|Ae|Af; // M + m =  | || |
    case 'm': return Aa|Ab|Ac|Af; //          |    |
    case 'N': return lookup('n');
    case 'n': return Ac|Ae|Ag;
    case 'O': return lookup('0');
    case 'o': return Ac|Ad|Ae|Ag;
    case 'P': return Aa|Ab|Ae|Af|Ag;
    case 'p': return lookup('P');
    case 'Q': return lookup('q');
    case 'q': return Aa|Ab|Ac|Af|Ag;
    case 'R': return lookup('r');
    case 'r': return Ae|Ag;
    case 'S': return Aa|Ac|Ad|Af|Ag;
    case 's': return lookup('S');
    case 'T': return lookup('t');
    case 't': return Ad|Ae|Af|Ag;
    case 'U': return Ab|Ac|Ad|Ae|Af;
    case 'u': return Ac|Ad|Ae;
    case 'V': return lookup('U');
    case 'v': return lookup('u');
    case 'W': return Ac|Ad|Ae|Af; // like M + m
    case 'w': return Ab|Ac|Ad|Ae; //
    case 'X': return lookup('H');
    case 'x': return lookup('H');
    case 'Y': return lookup('y');
    case 'y': return Ab|Ac|Ad|Af|Ag;
    case 'Z': return lookup('2');
    case 'z': return lookup('2');
    // symbols
    case ':': return Adt|Adb;
    case '.': return Adb;
    case '\'': return Adt;
    case '-': return Ag;
    case '_': return Ad;
    case '>': return Ac|Ad;
    case '<': return Ad|Ae;
    case '=': return Ad|Ag;
    // empty be default
    default: return 0;
  }
}