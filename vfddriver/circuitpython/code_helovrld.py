# This code can be put on a CircuitPython device to
# quickly display some text on the display over I2C.
import board
from time import sleep

# display address
address = 0x68

# init i2c peripheral
sleep(1)
i2c = board.STEMMA_I2C()
i2c.try_lock()

# shorthand to write chars
def display(data):
  i2c.writeto(address, data)

# infinite loop hello world
while True:
  display(b"HE LO")
  sleep(1)
  display(b"Vr Ld")
  sleep(1)
