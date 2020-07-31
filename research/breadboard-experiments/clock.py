#!/usr/bin/env python3

import board
import digitalio
import struct
import time
from time import sleep

spi = board.SPI()

# the individual grids for digits
DIGITS = [ 1, 2, 4, 8, 16 ]
d1 =  1 # 1st
d2 =  2 # 2nd
dd =  4 # dots
d3 =  8 # 3rd
d4 = 16 # 4th

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

#       a
#     ────
#  f │    │
#    │  g │ b   ■ dt
#     ────
#  e │    │
#    │    │ c   ■ db
#     ────
#       d

# some predefined characters
char = {
    "0": a|b|c|d|e|f,
    "1": b|c,
    "2": a|b|g|e|d,
    "3": a|b|c|d|g,
    "4": f|g|b|c,
    "5": a|f|g|c|d,
    "6": a|f|e|d|c|g,
    "7": a|b|c,
    "8": a|b|c|d|e|f|g,
    "9": a|b|c|d|g|f,
    ":": dt|db,
    " ": 0,
}

# how long to wait after each call to show()
import sys
DELAY = 0.006 if len(sys.argv) < 2 else float(sys.argv[1])

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

printed = set()

def Time():
  # get local time
  t = time.localtime()
  fmt = "%04d-%03d %02d:%02d" % (t.tm_year, t.tm_yday, t.tm_hour, t.tm_min)
  if fmt not in printed:
    printed.add(fmt)
    print(fmt)
  # display hours
  hh = "%02d" % t.tm_hour
  show(d1 | char[hh[0]])
  show(d2 | char[hh[1]])
  # blink dots every second
  show(dd | (char[":"] if (t.tm_sec % 2) == 0 else 0))
  # display minutes
  mm = "%02d" % t.tm_min
  show(d3 | char[mm[0]])
  show(d4 | char[mm[1]])

while True:
  Time()
