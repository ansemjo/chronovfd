import board, os, wifi, rtc, asyncio, time
from microcontroller import watchdog as wdt
from watchdog import WatchDogMode
from microcontroller import reset
import adafruit_connection_manager as connmgr
import adafruit_ntp as ntp

# init i2c peripheral
time.sleep(0.1)
i2c = board.STEMMA_I2C()
i2c.try_lock()

addr = 0x68

# shorthand to write data
def display(data):
  i2c.writeto(addr, data)
  
# set the brightness
def brightness(vref):
  i2c.writeto(addr, bytes([0xfa, vref])) 

# show a fatal error
def err(message):
  display("er r ")
  raise Exception(message)

# boot up sequence
print("Hello, chronovfd!")
#brightness(62) # almost off
display("00:00") # 00:00

# -- settings.toml sample --
# WIFI_PSK_mySSID = "superSecurePassword"
# NTP_SERVER = "3.de.pool.ntp.org"


# ------------------ clock ------------------ #

# format full datetime as a string
def datefmt(tm):
  year, mon, day, hour, min, sec, _, _, _ = tm
  return f"{year:04d}-{mon:02d}-{day:02d} {hour:02d}:{min:02d}:{sec:02d}"

# format time as hh:mm
def hhmm(tm):
  _, _, _, hour, min, sec, _, _, _ = tm
  colon = ":" if (sec % 2) == 0 else " "
  return f"{hour:02d}{colon}{min:02d}"

# continuously show RTC datetime on display
async def clockdisplay():
  while True:
    now = get_local_time()
    # print to console
    print(f"\r{datefmt(now)}", end="")
    # show on display; blink colon every second
    display(hhmm(now))
    # yield to other tasks
    await asyncio.sleep(0.1)

# fix time struct's wday and yday with a trip through unix timestamp
def fixtime(struct_time):
  return time.localtime(time.mktime(struct_time))

# check if returned time in in EU daylight savings window (CEST)
# DST rules for EU: last Sunday in March to last Sunday in October
def is_dst(utc_struct_time):
  year = utc_struct_time.tm_year
  # compute the last sunday in march and october of this year
  march31 = fixtime(time.struct_time((year, 3, 31, 0, 0, 0, 0, 0, -1)))
  march_last_sunday = 31 - ((march31.tm_wday + 1) % 7)
  oct31 = fixtime(time.struct_time((year, 10, 31, 0, 0, 0, 0, 0, -1)))
  oct_last_sunday = 31 - ((oct31.tm_wday + 1) % 7)
  # convert everything to timestamps for comparison
  now = time.mktime(utc_struct_time)
  dst_start = time.mktime(time.struct_time((year, 3, march_last_sunday, 1, 0, 0, 0, 0, -1)))
  dst_end   = time.mktime(time.struct_time((year, 10, oct_last_sunday,   1, 0, 0, 0, 0, -1)))
  return dst_start <= now <= dst_end

# return the local CET/CEST time
def get_local_time():
  now = rtc.RTC().datetime
  unix = time.mktime(now)
  offset = 7200 if is_dst(now) else 3600 # +2 CEST or +1 CET
  return time.localtime(unix + offset)  

# ------------------ wifi ------------------ #

# scan available networks
def scan():
  mac = ":".join([f"{e:02x}" for e in wifi.radio.mac_address])
  print(f"Your MAC is: {mac}")
  print("Scanning for wireless networks ..")
  ssids = set()
  networks = wifi.radio.start_scanning_networks()
  for n in networks:
    print(f"  {n.ssid}\t({n.rssi}dB, ch{n.channel})")
    ssids.add(n.ssid)
  wifi.radio.stop_scanning_networks()
  return ssids

# connect to some known wifi network
def connect():
  some = False
  for ssid in scan():
    # if we have a psk for this net, try to connect
    psk = os.getenv(f"WIFI_PSK_{ssid}")
    if psk != None:
      some = True
      try:
        print(f"Connecting to '{ssid}' .. ", end="")
        wifi.radio.connect(ssid, psk)
        print(f"ok! ipv4: {wifi.radio.ipv4_address}")
        # convert obtained address to numerical strings for display
        return [str(p) for p in wifi.radio.ipv4_address.packed]
      except Exception as e:
        print(f"fail! {e}")
  # end of the list, fail
  display("au th")
  time.sleep(2)
  if not some:
    raise Exception("Err: no known networks in range!")
  raise Exception("Err: could not connect to any network!")

# disconnect again
def disconnect():
  wifi.radio.stop_station()


# ------------------ ntpd ------------------ #

ntp_serv = os.getenv("NTP_SERVER", "0.de.pool.ntp.org")

fetch_in_progress = False
lastupdate = 0
ntpd = None

# fetch current time from an NTP server
def fetchtime():
  global fetch_in_progress, lastupdate, ntpd
  if fetch_in_progress: return
  else: fetch_in_progress = True
  try:
    # set watchdog timer to cancel if anything goes wrong
    wdt.timeout = 30
    wdt.mode = WatchDogMode.RAISE
    wdt.feed()

    # show current time with a single dot to signal update
    datestr = hhmm(rtc.RTC().datetime)
    datestr[2] = "."
    display(datestr)

    # connect to some known wifi
    if not wifi.radio.connected:
      connect()
      wdt.feed()
      pool = connmgr.get_radio_socketpool(wifi.radio)
      ntpd = ntp.NTP(pool, server=ntp_serv, socket_timeout=10)
      wdt.feed()

    # access ntp server
    print(f"Fetch NTP time from {ntp_serv} .. ", end="")
    now = ntpd.datetime
    wdt.mode = None
    print(f"ok! {datefmt(now)} UTC")
    rtc.RTC().datetime = now
    lastupdate = time.time()

  except Exception as e:
    # shouldn't be fatal here
    print(f"fetchtime failed: {e}")

  finally:
    fetch_in_progress = False
    wdt.mode = None


# update RTC when needed
async def timefetcher():
  await asyncio.sleep(20)
  while True:
    # sleep until next occurence of the update time
    target_sec = 3 * 3600 + 30 * 60 # 03:30 in the night
    now = time.localtime()
    current_sec = now.tm_hour * 3600 + now.tm_min * 60 + now.tm_sec
    wait_sec = (target_sec - current_sec) % 86400
    print("\ntimefetcher: sleeping for", wait_sec, "seconds until next update")
    await asyncio.sleep(wait_sec)
    fetchtime()

# update RTC on boot, if needed
async def bootupdate():
  global lastupdate
  await asyncio.sleep(1)
  if time.localtime().tm_year == 2000:
    now = datefmt(time.localtime())
    print(f"\nRTC time seems old: {now}.")
    fetchtime()
  else:
    print("\nRTC seems to be up-to-date.")
    lastupdate = time.time()

# ------------------ main ------------------ #

print("starting main!")
async def main():
  tasks = [ clockdisplay, bootupdate, timefetcher ]
  await asyncio.gather(*[ asyncio.create_task(t()) for t in tasks ])

try:
  asyncio.run(main())
except Exception as e:
  err(e)
