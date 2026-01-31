# This code can be put on a CircuitPython device to
# quickly display some text on the display over I2C.
import board
from time import sleep, monotonic as now

# init i2c peripheral
sleep(1)
i2c = board.I2C()
i2c.try_lock()

# shorthand to write data
def display(data):
  i2c.writeto(0x68, data)

# running text
def runner(text, delay):
  display("     ")
  for n in range(len(text)-4):
    display(text[n:n+4])
    sleep(delay)

# greetings with running text
runner("    CABLE TRASH    ", 0.12)

# blinking on/off last two digits quickly
def flash(prefix):
  for _ in range(5):
    display(prefix + "\x10\x10:") # off
    sleep(0.08)
    display(prefix + "88:") # on
    sleep(0.08)

# sleep until ..
def until(t):
  while now() < t: pass

# start counting
then = now() + 1
for p in range(10):
  page = f"P{p}"
  for s in range(99, 0, -1):
    display(page + f"{s:02d}:")
    until(then)
    then += 1
  flash(page)

# noise when we're done
display(b"\xff\xff\xff\xff\xff")
sleep(3)

# running text until reset
while True:
  # greetings with running text
  runner("    CABLE TRASH    ", 0.2)
