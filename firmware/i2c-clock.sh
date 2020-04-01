#!/usr/bin/bash

BUS=10 # find correct bus with: i2cdetect -l
DEV="0x20" # configured device address of vfd

while true; do

  # format HH:MM string
  HHMM=$(printf "%s" "$(date +%H%M)" | hexdump -e '/1 "0x%02x "')
  SECS=$([[ $(( "$(date +%-S)" % 2 )) -eq 0 ]] && echo 0x3a || echo 0x00)
  
  # transfer text to vfd buffer
  i2ctransfer -y $BUS w6@$DEV 0 $HHMM $SECS
  sleep 0.1

done
