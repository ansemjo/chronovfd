#!/usr/bin/bash

BUS="$(i2cdetect -l | awk '/i2c-tiny-usb/{ gsub("i2c-", "", $1); print $1; }')"
DEV="0x20" # configured device address of vfd

while true; do

  # format HH:MM string
  HHMM="$(printf "%s" "$(date "+%H%M")" | hexdump -ve '/1 "0x%02x "')"
  SECS="$([[ $(( "$(date "+%-S")" % 2 )) -eq 0 ]] && echo "0x3a" || echo "0x00")"
  
  # transfer text to vfd buffer
  i2ctransfer -y "$BUS" "w6@$DEV" 0 $HHMM "$SECS"
  sleep 0.1

done
