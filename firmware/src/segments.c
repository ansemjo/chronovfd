#include <stdio.h>
#include "segments.h"

// character lookup for segment mapping
uint16_t lookup(char ch) {
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
    case 'a': return lookup('A');
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
    case '~': return Aa|Ad|Ag;
    case ';': return Ac|Ae;
    // empty be default
    default: return 0;
  }
}
