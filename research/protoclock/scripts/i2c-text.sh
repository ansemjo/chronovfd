#!/usr/bin/bash
# save the first 5 bytes
input=$(head -c 5)

BUS="$(i2cdetect -l | awk '/i2c-tiny-usb/{ gsub("i2c-", "", $1); print $1; }')"
DEV="0x20" # configured device address of vfd

TEXT="$(printf '%-5s' "$input" | hexdump -ve '/1 "0x%02x "')"
i2ctransfer -y "$BUS" "w6@$DEV" 0 $TEXT
