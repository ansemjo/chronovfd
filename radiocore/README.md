# radiocore
This is a redesign of the [`si4706_radio_eval`](../experiments/si4706_radio_eval/) experiment, meant to be a "clockcore v2" to control the standalone vfddriver.

Notably, it includes:

* **Skyworks Si4706** FM radio receiver with RDS decoder, to get time signal "from the air"
* **DS1338 RTC** with coin cell battery backup, to keep accurate time over long periods and without power
* **ESP32-C3** as a simple-to-program-over-USB controller chip, which conveniently *also* includes WiFi capability, if you prefer NTP over RDS
* vertical **GCT USB4145** USB-C connector, so a final assembly with two sandwiched boards will have the cable coming in from the back

_**2026-02-03:** I ordered the boards and am waiting to try if the CircuitPython firmware from the experiment will work on this chip as well. (**edit:** yes, it does!)_

![](radiocore-render.png "A KiCAD rendering of the radiocore board. Top-left: an ESP32-C3 WROOM module. Bottom: DS1338 RTC with a crystal and the Skyworks Si4706 next to it. Right edge: vertical USB-C connector.")
![](radiocore-front.png "KiCAD screenshot showing the traces on the front side. Visible is a recessed ground fill in the middle and a PCB trace antenna around the edge for radio reception.")

### errata

While the overall function works (reception is even better than expected, to be honest), I also made a few major mistakes:

* The Neopixel is supplied with VBUS (~ 5V) but only gets logic level 3.3V on its data pin from the ESP32. When the VBUS supply voltage is slightly over 5V (like 5.2–5.3V), the Neopixel consistenly stays dark. When it is exactly at or slightly under 5V, you can *sometimes* get it to work. At 5V, the SK6812 requires a 3.6V logic level per the datasheet. Oops.

* The UPDI connection (pin 5 on the side header) is connected directly to the ESP32 without level shifting or current limit. Since this pin is at 5V logic level on the ATtiny, you could damage the ESP32-C3 if it is ever connected during programming of the `vfddriver`. Use a Qwiic cable only or just solder the first four pins to be safe.

* While it works out, the cutout under the Qwiic connector is a little close to properly insert the plug while the cable is already weaved through the hole.

* In the previous `vfddriver` firmware, the display's address was `0x68` and thus collided with the RTC. I've moved the VFD to address `0x42` and you can still reconfigure it using the `user_row`, of course.
