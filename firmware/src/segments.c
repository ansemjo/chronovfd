// Copyright (c) 2020 Anton Semjonov
// Licensed under the MIT License

#include <stdint.h>
#include "segments.h"

// array of grids for looping
const uint16_t grids[GRIDS] = { G1, G2, G3, G4, G5 };

// double-lookup for brightness corrections
const uint8_t gridpos[5] = { 0, 1, 2, 3, 4 };

// character segment_lookup for segment mapping
uint16_t segment_lookup(char ch) {
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
    case 'a': return segment_lookup('A');
    case 'B': return segment_lookup('b');
    case 'b': return Ac|Ad|Ae|Af|Ag;
    case 'C': return Aa|Ad|Ae|Af;
    case 'c': return Ad|Ae|Ag;
    case 'D': return segment_lookup('d');
    case 'd': return Ab|Ac|Ad|Ae|Ag;
    case 'E': return Aa|Ad|Ae|Af|Ag;
    case 'e': return segment_lookup('E');
    case 'F': return Aa|Ae|Af|Ag;
    case 'f': return segment_lookup('F');
    case 'G': return segment_lookup('6');
    case 'g': return segment_lookup('9');
    case 'H': return Ab|Ac|Ae|Af|Ag;
    case 'h': return Ac|Ae|Af|Ag;
    case 'I': return Ae|Af;
    case 'i': return Ae;
    case 'J': return Ab|Ac|Ad;
    case 'j': return segment_lookup('J');
    case 'K': return segment_lookup('k');
    case 'k': return Aa|Ac|Ae|Af|Ag;
    case 'L': return Ad|Ae|Af;
    case 'l': return segment_lookup('I'); //           _  _
    case 'M': return Aa|Ab|Ae|Af; // M + m =  | || |
    case 'm': return Aa|Ab|Ac|Af; //          |    |
    case 'N': return segment_lookup('n');
    case 'n': return Ac|Ae|Ag;
    case 'O': return segment_lookup('0');
    case 'o': return Ac|Ad|Ae|Ag;
    case 'P': return Aa|Ab|Ae|Af|Ag;
    case 'p': return segment_lookup('P');
    case 'Q': return segment_lookup('q');
    case 'q': return Aa|Ab|Ac|Af|Ag;
    case 'R': return segment_lookup('r');
    case 'r': return Ae|Ag;
    case 'S': return Aa|Ac|Ad|Af|Ag;
    case 's': return segment_lookup('S');
    case 'T': return segment_lookup('t');
    case 't': return Ad|Ae|Af|Ag;
    case 'U': return Ab|Ac|Ad|Ae|Af;
    case 'u': return Ac|Ad|Ae;
    case 'V': return segment_lookup('U');
    case 'v': return segment_lookup('u');
    case 'W': return Ac|Ad|Ae|Af; // like M + m
    case 'w': return Ab|Ac|Ad|Ae; //
    case 'X': return segment_lookup('H');
    case 'x': return segment_lookup('H');
    case 'Y': return segment_lookup('y');
    case 'y': return Ab|Ac|Ad|Af|Ag;
    case 'Z': return segment_lookup('2');
    case 'z': return segment_lookup('2');
    // symbols
    case ':': return Adt|Adb;
    case '.': return Adb;
    case '\'': return Adt;
    case '-': return Ag;
    case '_': return Ad;
    case '>': return Ac|Ad;
    case '<': return Ad|Ae;
    case '=': return Ad|Ag;
    case '~': return Aa|Ad|Ag;
    case ';': return Ac|Ae;
    // empty be default
    default: return 0;
  }
}
