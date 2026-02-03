# radiocore
This is a redesign of the [`si4706_radio_eval`](../experiments/si4706_radio_eval/) experiment, meant to be a "clockcore v2" to control the standalone vfddriver.

Notably, it includes:

* **Skyworks Si4706** FM radio receiver with RDS decoder, to get time signal "from the air"
* **DS1338 RTC** with coin cell battery backup, to keep accurate time over long periods and without power
* **ESP32-C3** as a simple-to-program-over-USB controller chip, which conveniently *also* includes WiFi capability, if you prefer NTP over RDS
* vertical **GCT USB4145** USB-C connector, so a final assembly with two sandwiched boards will have the cable coming in from the back

_**2026-02-03:** I ordered the boards and am waiting to try if the CircuitPython firmware from the experiment will work on this chip as well._

![](radiocore-render.png "A KiCAD rendering of the radiocore board. Top-left: an ESP32-C3 WROOM module. Bottom: DS1338 RTC with a crystal and the Skyworks Si4706 next to it. Right edge: vertical USB-C connector.")
![](radiocore-front.png "KiCAD screenshot showing the traces on the front side. Visible is a recessed ground fill in the middle and a PCB trace antenna around the edge for radio reception.")