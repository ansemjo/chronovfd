# protoclock

This is the firmware for my protoboard version of the clock
based on an ATtiny84 and a DS3231 module connected over I2C.

The jumper is read on startup:

* `LOW` (closed) --> I2C mode, address `0x20`
* `HIGH` (open) --> clock mode

In I2C mode you can use the scripts in the sibling directory
to push content onto the display. They're written with an
i2c-tiny-usb in mind.
