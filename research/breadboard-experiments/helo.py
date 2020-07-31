#!/usr/bin/env python3

import board
import digitalio
import struct
from time import sleep

spi = board.SPI()

# the individual grids for digits
DIGITS = [ 1, 2, 4, 8, 16 ]
d1 = 1
d2 = 2
d3 = 8
d4 = 16

# segment bitmask
ALLSEGMENTS = 0b11111111100000
a = 1 << 13
b = 1 << 11
c = 1 << 8
d = 1 << 10
e = 1 << 7
f = 1 << 12
g = 1 << 6
dt = 1 << 5
db = 1 << 9

# some predefined characters
H = b|c|e|f|g
E = a|d|e|f|g
L = d|e|f
O = a|b|c|d|e|f

# how long to wait after each call to show()
DELAY = 0.003

# strobe pin to latch register to outputs
strobe = digitalio.DigitalInOut(board.D6)
strobe.direction = digitalio.Direction.OUTPUT
def latch():
	strobe.value = True
	strobe.value = False

# display a bitmask of digit + segments
def show(u):
	spi.write(struct.pack(">H", u))
	latch()
	sleep(DELAY)

# display greeting
def HELO():
	while True:
		show(d1 | H)
		show(d2 | E)
		show(d3 | L)
		show(d4 | O)

HELO()
