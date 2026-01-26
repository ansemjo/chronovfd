print()
print("    _____ ___________  ____")
print("   / __(_) / /_  / _ \/ __/")
print("  _\ \/ /_  _// / // / _ \ ")
print(" /___/_/ /_/ /_/\___/\___/ ")
print()

import os, board, time, math
from rtc import RTC
from sparkfun_serlcd import Sparkfun_SerLCD_I2C
from adafruit_bus_device.i2c_device import I2CDevice
import neopixel

from time import sleep
from supervisor import ticks_ms
rtc = RTC()

# disable automatic reloads
from supervisor import runtime
runtime.autoreload = bool(os.getenv("autoreload", 1))

# start using the I2C connection
i2c = board.STEMMA_I2C()
lcd = Sparkfun_SerLCD_I2C(i2c)

# use the onboard pixel
p = neopixel.NeoPixel(board.NEOPIXEL, 1)
p.brightness = 0.5
p.fill((0, 0, 0))

# colors for lcd
blueish = (20, 20, 255)
greenish = (20, 255, 20)
redish = (255, 20, 20)

# setup the sparkfun lcd
lcd.set_fast_backlight_rgb(*blueish)

def blink(r, g, b, t = 0.1):
  p.fill((r, g, b))
  time.sleep(t)
  p.fill((0, 0, 0))
blink(0, 255, 0)

# Si4706, alternative address with ^SEN = 1
addr = 0b1100011 # 99

# wait for device to appear
while not i2c.try_lock(): pass
while addr not in i2c.scan(): pass
i2c.unlock()
fm_device = I2CDevice(i2c, addr)
print(f"--> found Si47xx device on address 0x{addr:02x}")

# ------------------------------------------------------------------

# write a command to device, optionally read back a buffer
def write_command(*cmdargs, read=None, wait=True):
  with fm_device as fm:
    if read == None:
      fm.write(bytes(cmdargs))
    else:
      fm.write_then_readinto(bytes(cmdargs), read)
  if wait:
    while not clear_to_send(): pass

# list a few useful commands from AN332 §5
POWER_UP   = 0x01
POWER_DOWN = 0x11
GET_REV    = 0x10
SET_PROPERTY   = 0x12
GET_INT_STATUS = 0x14
FM_TUNE_FREQ   = 0x20
FM_SEEK_START  = 0x21
FM_TUNE_STATUS = 0x22
FM_RSQ_STATUS  = 0x23
FM_RDS_STATUS  = 0x24

# bitmasks for status bits
MASK_CTS = 0b10000000 # clear-to-send
MASK_ERR = 0b01000000 # error status
MASK_RDS = 0b00000100 # RDS interrupt
MASK_RSQ = 0b00001000 # signal quality interrupt
MASK_STC = 0b00000001 # seek/tune complete

# read registers into a given buffer
def readbuf(buf):
  with fm_device as fm:
    fm.readinto(buf)
    return buf

# read this many bytes into a fresh buffer
def read(n):
  with fm_device as fm:
    buf = bytearray(n)
    fm.readinto(buf)
    return buf

# read the status register and parse it to dict
def status(status = None):
  if status is None:
    status = read(1)[0]
  if status & MASK_ERR:
    raise ValueError(f"error status detected! {status:08b}")
  return {
    MASK_CTS: bool(status & MASK_CTS), # clear to send
    MASK_ERR: bool(status & MASK_ERR), # errors present
    MASK_RDS: bool(status & MASK_RDS), # RDS interrupt (not really)
    MASK_RSQ: bool(status & MASK_RSQ), # signal quality measurement
    MASK_STC: bool(status & MASK_STC), # seek / tune complete
  }

# are we clear-to-send? (CTS bit in status)
def clear_to_send():
  return status()[MASK_CTS]

# is RDS data ready? (RDSINT bit in response)
def rds_ready():
  interrupts = bytearray(1)
  write_command(GET_INT_STATUS, read=interrupts)
  return status(interrupts[0])[MASK_RDS]

# is the current seek/tune complete?
def fm_tune_status(cancel = False, clear = False):
  response = bytearray(8)
  write_command(FM_TUNE_STATUS, (cancel << 1) + clear, read=response)
  # return parsed responses
  done = status(response[0])[MASK_STC]
  band  = response[1] & 0b10000000 # hit band limit or wrapped around
  valid = response[1] & 0b00000001 # channel is valid per current thresholds
  freq = (response[2] << 8) + response[3] # tuned frequency
  rssi =  response[4] # received signal strength indicator (dBµV)
  snr  =  response[5] # signal-to-noise ratio (dB)
  return done, freq, valid, band, rssi, snr

# get current signal status
def fm_status():
  response = bytearray(8)
  write_command(FM_RSQ_STATUS, 0x00, read=response)
  # return parsed responses
  smute = response[2] & 0b00001000 # soft-mute engaged
  valid = response[2] & 0b00000001 # channel is valid per current thresholds
  pilot = response[3] & 0b10000000 # stereo pilot is present
  rssi =  response[4] # received signal strength indicator (dBµV)
  snr  =  response[5] # signal-to-noise ratio (dB)
  return smute, valid, pilot, rssi, snr


# repeatedly get tuning status until the seek is complete
def wait_for_seek(log = False):
  lastfreq = 0
  while True:
    done, freq, valid, band, rssi, snr = fm_tune_status()
    if log and freq != lastfreq:
      print(f"\33[2K\r· {(freq/100):6.2f} MHz [{rssi:3d} dBµV, SNR: {snr:3d}] {'ok' if valid else ''}", end="")
      lcd.set_cursor(0, 0)
      lcd.write(f"{(freq/100):6.2f}  {rssi:2d}uV{snr:2d}dB")
      lastfreq = freq
    if done: break
    time.sleep(0.01)
  if log: print()
  return fm_tune_status(clear=True)

# power the chip down
def power_down():
  write_command(POWER_DOWN, wait=True)

# power the chip up
def power_up(crystal = True):
  write_command(POWER_UP,
              crystal << 4, # enable crystal oscillator
              0x00, # no audio output at all
              wait=True)

# get chip information
def get_rev():
  write_command(GET_REV, wait=True)
  r = read(16)
  chip = f"Si47{r[1]:02x}"
  firmware  = f"{chr(r[2])}.{chr(r[3])}"
  component = f"{chr(r[6])}.{chr(r[7])} ({chr(r[8])})"
  return chip, firmware, component

# select pcb antenna on LPI pin
def use_pcb_antenna():
  print("--> use TXO/LPI pin for antenna input")
  write_command(SET_PROPERTY, 0x00, 0x11, 0x07, 0x00, 0x01)

# select wire antenna on FMI pin
def use_wire_antenna():
  print("--> use FMI pin for antenna input")
  write_command(SET_PROPERTY, 0x00, 0x11, 0x07, 0x00, 0x00)

# seek to the next good channel
def seek(up = True, wrap = True):
  print("--> seek to next channel")
  write_command(FM_SEEK_START, (up << 3) + (wrap << 2))
  return wait_for_seek(log = True)

# tune to a specified frequency
def tune(freq):
  print(f"--> tune to specified frequency: {(freq/100):6.2f}")
  write_command(FM_TUNE_FREQ, 0x00, freq >> 8, freq & 0xff, 0x00, wait=False)
  time.sleep(0.1)
  return wait_for_seek(log = True)

# do a full frequency scan manually
def full_frequency_scan(print_above = 5):
  print("\n--> perform a FULL FREQUENCY SCAN for good channels")
  best = (0, 0)
  for freq in range(8750, 10790, 5):
    # tune to given frequency, automatic antenna capacitor
    write_command(FM_TUNE_FREQ, 0x00, freq >> 8, freq & 0xff, 0x00, wait=False)
    time.sleep(0.1)
    done, freq, valid, band, rssi, snr = wait_for_seek(log = False)
    bar = "#" * snr
    if snr > best[0]: best = (snr, freq)
    if snr > print_above:
      print(f"· {(freq/100):6.2f} MHz [{rssi:3d} dBµV, SNR: {snr:3d}] {'ok' if valid else '  '} {bar}")
      lcd.set_cursor(0, 0)
      lcd.write(f"{(freq/100):6.2f}  {rssi:2d}uV{snr:2d}dB")
  return best[1]

# ------------------------------------------------------------------

# always try to turn device off first thing
print("--> force POWER_DOWN")
power_down()
time.sleep(0.5)

# turn device (back) on
print("--> perform POWER_UP")
power_up(crystal = True)

# get chip information
chip, firmware, component = get_rev()
print(f"--> {chip} (firmware: {firmware}, hardware: {component})")

# set some properties
print("--> set FM_DEEMPHASIS = 50µs (EU)")
write_command(SET_PROPERTY, 0x00, 0x11, 0x00, 0x00, 0x01)
print("--> set FM_MAX_TUNE_ERROR = 40 kHz")
write_command(SET_PROPERTY, 0x00, 0x11, 0x08, 0x00, 0x28)
print("--> set FM_SEEK_FREQ_SPACING = 50 kHz")
write_command(SET_PROPERTY, 0x00, 0x14, 0x02, 0x00, 0x05)

thresh_snr  = 10
thresh_rssi = 20
print(f"--> set FM_SEEK_TUNE_SNR_THRESHOLD = {thresh_snr:2d} dB")
write_command(SET_PROPERTY, 0x00, 0x14, 0x03, 0x00, thresh_snr)
print(f"--> set FM_SEEK_TUNE_RSSI_THRESHOLD = {thresh_rssi:2d} dBµV")
write_command(SET_PROPERTY, 0x00, 0x14, 0x04, 0x00, thresh_rssi)

#print("\n--> configure RDS receiver and start listening ...")
print("--> set FM_RDS_INT_SOURCE on data receive")
write_command(SET_PROPERTY, 0x00, 0x15, 0x00, 0x00, 0x01) # interrupt on data receive
print("--> set FM_RDS_INT_FIFO_COUNT = 4 groups")
write_command(SET_PROPERTY, 0x00, 0x15, 0x01, 0x00, 0x04) # store four groups in FIFO
print("--> set FM_RDS_CONFIDENCE to a higher value")
write_command(SET_PROPERTY, 0x00, 0x15, 0x03, 0x33, 0x33) # default: 0x1111

print("--> set FM_RDS_CONFIG to enable RDS and set error tolerance")
#write_command(SET_PROPERTY, 0x00, 0x15, 0x02, 0b_11_00_00_00, 0x01) # zero errors in B-C
#write_command(SET_PROPERTY, 0x00, 0x15, 0x02, 0b_11_01_01_01, 0x01) # allow 1-2 bit errors in B-C
write_command(SET_PROPERTY, 0x00, 0x15, 0x02, 0b_11_01_10_10, 0x01) # 3122 correction (be sure about type)
#write_command(SET_PROPERTY, 0x00, 0x15, 0x02, 0b_11_10_10_10, 0x01) # allow 3-5 bit errors in B-C
#write_command(SET_PROPERTY, 0x00, 0x15, 0x02, 0b_11_10_11_11, 0x01) # 3233 error correction
#write_command(SET_PROPERTY, 0x00, 0x15, 0x02, 0b_11_11_11_11, 0x01) # allow UNCORRECTED errors!

# select antenna to use
use_pcb_antenna()
#use_wire_antenna()

# frequency sweep to find good stations
#freq = full_frequency_scan()
# or seek to next good channel
#seek()

# --> known-good stations near Hamburg
NDR_2    =  8760 # NDR 2
DLF      =  8870 # Deutschlandfunk
NDR_903  =  9030 # NDR 90.3
NDR_INFO =  9230 # NDR Info
DELTA    =  9340 # delta Radio
ENERGY   =  9710 # ENERGY
KLASSIK  =  9810 # Klassik Radio
RSH_SUED = 10000 # R.SH Radio Schleswig-Holstein
RSH_LA   = 10250 # R.SH (Lauenburg)
RADIOHH  = 10360 # Radio Hamburg
BYTEFM   = 10400 # ByteFM
DELTA_LA = 10560 # delta Radui (Lauenburg)
ROCKHH   = 10680 # Rock Antenne Hamburg

tune(NDR_903)
#seek()

# store information received through RDS
def reset_station():
  global station, message, message_ab
  station = bytearray("        ")
  message = bytearray([ord(" ") for _ in range(64)])
  message_ab = -1 # detect message change to print
reset_station()

# clean decode which ignores everything that isn't a useful character
def text(buf):
  buf = [ chr(b) if b == 0x0a or b == 0x0d or (b >= 32 and b < 127) else " " for b in buf ]
  return "".join(buf).splitlines()[0]

# format localtime in iso-like string
def clock():
  (y, M, d, h, m, s, wd, yd, dst) = time.localtime()
  return f"{y:04d}{M:02d}{d:02d} {h:02d}{m:02d}{s:02d}"

# buffer for RDS group data
rds = bytearray(13)

# configure button for seeking
from digitalio import DigitalInOut, Direction, Pull
btn = DigitalInOut(board.BUTTON)
btn.direction = Direction.INPUT
btn.pull = Pull.UP

# save current start time
last_seek = time.time()
last_rdstime = time.time()
last_clockface = time.time()
last_rssiprint = time.time()
is_red = False

# endless loop to debug RDS reception
while True:
  now = time.time()

  # if last clock sync is more than a minute ago, show red
  if not is_red and (now - last_seek) > 60 and (now - last_rdstime) > 600:
    is_red = True
    lcd.set_fast_backlight_rgb(*redish)

  # update clock every second
  if (now - last_clockface) > 1:
    last_clockface = now
    lcd.set_cursor(0, 1)
    lcd.write(clock())

  # check button or time to re-seek
  if not btn.value or (now - last_rdstime) > 300:
    print()
    is_red = False
    last_seek = now
    lcd.set_fast_backlight_rgb(*blueish)
    reset_station()
    seek()
    time.sleep(1)

  # update signal status
  if (now - last_rssiprint) > 0.5:
    last_rssiprint = now
    smute, valid, pilot, rssi, snr = fm_status()
    lcd.set_cursor(8, 0)
    lcd.write(f"{rssi:2d}uV{snr:2d}dB")
    #print(f"· RSSI: {rssi:3d} dBµV, SNR: {snr:3d}, mute: {smute}  {'ok' if valid else '  '}")

  # wait for RDS data and read groups into buffer
  if not rds_ready(): continue
  write_command(FM_RDS_STATUS, 0x01, read=rds)

  # not due to rds received
  if not (rds[1] & 0x01): continue

  # rds is not currently synchronized
  if not (rds[2] & 0x01): continue

  # block A should always be the station code
  sta = int.from_bytes(rds[4:6], "big")

  # check how many errors there were
  errs = [
    (rds[12] >> 6) & 0b11,
    (rds[12] >> 4) & 0b11,
    (rds[12] >> 2) & 0b11,
    (rds[12] >> 0) & 0b11,
  ]
  errs = "".join("·" if e == 0 else str(int(e)) for e in errs)

  # block B contains data information
  blockb = int.from_bytes(rds[6:8], "big")
  b_group = (blockb & 0xf000) >> 12

  # 0x00 --> station name decoder
  if b_group == 0x00:
    offset = (rds[7] & 0x03) * 2
    station[offset    ] = rds[10]
    station[offset + 1] = rds[11]
    # clean up before print
    rds[8] = 32
    rds[9] = 32

  # print raw data after decoding station
  print(f"\33[2K\r[{text(station)}] {sta:X} {b_group:02x} (err {errs}) {bytes(rds[8:12])}", end="")
  lcd.set_cursor(0, 0)
  lcd.write(text(station)[:8])
  blink(0, 0, 50, t=0.05)

  # 0x02 --> radio message decoder
  if b_group == 0x02:
    b0 = (rds[6] & 0b00001000) >> 3
    ab = (rds[7] & 0b00010000) >> 4

    # if message switched, print previous buffer
    if message_ab == -1: message_ab = ab
    elif message_ab != ab:
      #blink(0, 0, 50)
      #print(f"\n[{text(station)}] {text(message)}")
      message_ab = ab

    # set bytes at proper offset
    offset = (rds[7] & 0x0f) * 4
    message[offset + 0] = rds[ 8]
    message[offset + 1] = rds[ 9]
    message[offset + 2] = rds[10]
    message[offset + 3] = rds[11]


  # 0x04 --> date and time (clock) signal
  if b_group == 0x04:

    # collect payload bits
    # https://en.wikipedia.org/wiki/Radio_Data_System#Group_type_4_%E2%80%93_Version_A_%E2%80%93_Clock_time_and_date
    julian = ((rds[ 7] & 0x03) << 15) + ( rds[ 8] << 7) + (rds[9] >> 1)
    hrs = ((rds[ 9] & 0x01) <<  4) + ((rds[10] & 0xf0) >> 4)
    mns = ((rds[10] & 0x0f) <<  2) + ((rds[11] & 0xc0) >> 6)
    off = (-1 if (rds[11] & 0x10) else 1) * (rds[11] & 0x0f)

    # calculate the proper time with offset
    # based on Konrad Kosmatka's librdsparser code in:
    # https://github.com/kkonradpl/librdsparser/blob/master/src/ct.c
    mns += math.fmod(off, 2) * 30
    if mns >= 60: hrs += 1
    if mns  <  0: hrs -= 1
    mns = int(mns % 60)
    hrs += int(off / 2)
    if hrs >= 24: julian += 1
    if hrs  <  0: julian -= 1
    hrs = int(hrs % 24)

    # calculate year-month-day from julian
    year = int((julian * 100 - 1507820) / 36525)
    year_tmp = int((year * 36525) / 100)
    month = int(((julian * 100 - 1495610) - year_tmp * 100) * 100 / 306001)
    month_tmp = int(month * 306001 / 10000)
    day = julian - 14956 - year_tmp - month_tmp;
    k = 1 if month == 14 or month == 15 else 0;
    year = 1900 + year + k;
    month = month - 1 - k*12;

    # print datetime line and update rtc
    blink(0, 255, 0)
    print(f" --> {year:04d}-{month:02d}-{day:02d} {hrs:02d}:{mns:02d} ({off/2:.1f})")
    rtc.datetime = time.struct_time((year, month, day, hrs, mns, 0, -1, -1, -1))
    is_red = False
    last_rdstime = now
    lcd.set_fast_backlight_rgb(*greenish)


