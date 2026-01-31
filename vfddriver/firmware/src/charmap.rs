use crate::segments::*;

pub fn character(char: u8) -> u16 {

    if char == 0xff { return 0xffff }

    // treat as raw segment bitmask when outside printable characters
    if char >= 0x80 { return sevensegment(char) }

    // otherwise lookup mapping here
    match char {
        
        // numbers
        0x00 | b'0' => Aa|Ab|Ac|Ad|Ae|Af,
        0x01 | b'1' => Ab|Ac,
        0x02 | b'2' => Aa|Ab|Ag|Ae|Ad,
        0x03 | b'3' => Aa|Ab|Ac|Ad|Ag,
        0x04 | b'4' => Af|Ag|Ab|Ac,
        0x05 | b'5' => Aa|Af|Ag|Ac|Ad,
        0x06 | b'6' => Aa|Af|Ae|Ad|Ac|Ag,
        0x07 | b'7' => Aa|Ab|Ac,
        0x08 | b'8' => Aa|Ab|Ac|Ad|Ae|Af|Ag,
        0x09 | b'9' => Aa|Ab|Ac|Ad|Ag|Af,

        // alphabet
        b'A' => Aa|Ab|Ac|Ae|Af|Ag,
        b'a' => character(b'A'),
        b'B' => character(b'b'),
        b'b' => Ac|Ad|Ae|Af|Ag,
        b'C' => Aa|Ad|Ae|Af,
        b'c' => Ad|Ae|Ag,
        b'D' => character(b'd'),
        b'd' => Ab|Ac|Ad|Ae|Ag,
        b'E' => Aa|Ad|Ae|Af|Ag,
        b'e' => character(b'E'),
        b'F' => Aa|Ae|Af|Ag,
        b'f' => character(b'F'),
        b'G' => character(b'6'),
        b'g' => character(b'9'),
        b'H' => Ab|Ac|Ae|Af|Ag,
        b'h' => Ac|Ae|Af|Ag,
        b'I' => Ae|Af,
        b'i' => Ae,
        b'J' => Ab|Ac|Ad,
        b'j' => character(b'J'),
        b'K' => character(b'k'),
        b'k' => Aa|Ac|Ae|Af|Ag,
        b'L' => Ad|Ae|Af,
        b'l' => character(b'I'),
        b'M' => Aa|Ab|Ae|Af, // M + m = |-||-|
        b'm' => Aa|Ab|Ac|Af, //         |    |
        b'N' => character(b'n'),
        b'n' => Ac|Ae|Ag,
        b'O' => character(b'0'),
        b'o' => Ac|Ad|Ae|Ag,
        b'P' => Aa|Ab|Ae|Af|Ag,
        b'p' => character(b'P'),
        b'Q' => character(b'q'),
        b'q' => Aa|Ab|Ac|Af|Ag,
        b'R' => character(b'r'),
        b'r' => Ae|Ag,
        b'S' => Aa|Ac|Ad|Af|Ag,
        b's' => character(b'S'),
        b'T' => character(b't'),
        b't' => Ad|Ae|Af|Ag,
        b'U' => Ab|Ac|Ad|Ae|Af,
        b'u' => Ac|Ad|Ae,
        b'V' => character(b'U'),
        b'v' => character(b'u'),
        b'W' => Ac|Ad|Ae|Af, // like M + m = |    |
        b'w' => Ab|Ac|Ad|Ae, //              |_||_|
        b'X' => character(b'H'),
        b'x' => character(b'H'),
        b'Y' => character(b'y'),
        b'y' => Ab|Ac|Ad|Af|Ag,
        b'Z' => character(b'2'),
        b'z' => character(b'2'),

        // symbols
        b':' | b';' => Adt|Adb,
        b'.' | b',' => Ac|Adb,
        b'\'' => Af|Adt,
        b'"' => Af|Ab,
        b'-' => Ag,
        b'_' => Ad,
        b'+' => Ab|Ac|Ag,
        b'*' => Aa|Ab|Af|Ag, // ° (degree)
        b'>' => Ac|Ad,
        b'<' => Ad|Ae,
        b'=' => Ad|Ag,
        b'|' => Af|Ae,
        b'!' => Ab|Ac,
        b'?' => Aa|Ab|Ag|Ae,
        b'(' | b'[' => Aa|Af|Ae|Ad,
        b')' | b']' => Aa|Ab|Ac|Ad,
        b'/' => Ab|Ag|Ae,
        b'\\' => Af|Ag|Ac,

        // empty by default
        _ => 0,

    }
}
