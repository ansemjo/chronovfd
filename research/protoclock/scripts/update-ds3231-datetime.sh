#!/usr/bin/bash

# set the current date in a DS3231 RTC via i2c
BUS="$(i2cdetect -l | awk '/i2c-tiny-usb/{ gsub("i2c-", "", $1); print $1; }')"
DEV="0x68"

# print before values
echo    "        sec  min  hr   wkd  day  mon  year"
echo -n "before: "
i2ctransfer -y "$BUS" "w1@$DEV" 0 r7

# write current date
i2ctransfer -y "$BUS" "w8@$DEV" 0 $(date "+0x%S 0x%M 0x%H 0x%02u 0x%d 0x%m 0x%y")

# print after values
echo -n "after:  "
i2ctransfer -y "$BUS" "w1@$DEV" 0 r7
