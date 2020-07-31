# VFD Clock

I bought some old Russian / Ukranian stock of **IVL2-7/5** tubes. These are **V**acuum **F**lourescent **D**isplays used in old alarm clocks.

The basic principle has a cathode filament wire and several phosphorous-coated anode segents with section grids above them. A low voltage has to be supplied to the cathode to emit electrons which are attracted to a higher voltage on the anode grids and segments. When an electron hits the coating of a segment, it lights up in a blue-ish colour.

## Thoughts

Some thoughts on a select number of topics, when building such a clock:

### Realtime Clock

There's basically two approaches .. use a microcontrller with a sufficiently precise crystal and set the time on every boot, e.g. via NTP on an Espressif chip .. or use an onboard RTC module, which holds the time with a little button cell.

I think I favor the no-internet-of-shit approach, which uses only an offline microcontroller and can keep its time while it is disconnected from power. Either implement a simple three-button interface to set the clock or write a serial protocol to interface with the chip in-system.

### Microcontroller

As descibed above, I funfamentally need to make the choice between a connected device like an ESP8266 or a chip from the ATMega/ATTiny family.

And I tend towards the latter, specifically either an ATMega328P or an ATTiny84. They use 5 V logic levels, so I don't need any level shifting with my desired power source. And the project is just a damn clock after all! If I decide to do another design with a larger dot-matrix VFD, then a connected device might come in handy. For now though ..

I'd also like to experiment with barebones microchips. The Espressif boards are too much of a *complete module* for my tastes here.

### Power Source

There's a few designs which only use a 1.5 V battery and the kinda work .. albeit not for long. That's cool. But I would rather use a bigger battery, it doesn't need to be a wristwatch after all.

In any case, I think I would like to assume a regulated 5 V rail from which I derive all other voltages that I need. This allows me to solder a micro-USB port somewhere and run it off a powerbank. Also, most LiPo daughterboards appear to be interfacing at 5 V, so I could just put some sort of JST connector somewhere and not bother with charging logic at all.

### High-voltage Driver

Simply use a regulated boost converter, duh'. This part seems to be really simple.

### Filament Driver

The filament usually expects AC voltage to mitigate the effects of luminance gradients caused by the DC voltage drop in the filament. When one side has a lower potential than the other this might cause a visible gradient. On the one hand the voltage for this tube is relatively low to begin with and the tube is rather short. On the other hand a proper AC circuit is relatively complex. Just looking at the VFD PSU design by Rolo, the coupling capacitors would be the next biggest component after the anode multiplexer chip - weird.

Either way, this driver circuit needs to be regulated at the correct voltage. If I just pump 3.3 V into it continiously, it will eventually burn out or drastically reduce the lifetime. This seems to concern the RMS voltage however, so pulsing with the correct width could be feasible ..

I would like to have power circuits that do not depend on the microcontroller to provide correct oscillations. An ATTiny-controlled H-bridge would probably remain open in one direction if I program the microcontroller in-system .. eek. Ideally, the board should be usable as a serial-interface to the VFD, so it should not depend on any microcontroller.

There's a number of different approaches here:

* Just use **DC**.

  * Use a regulated converter to provide the correct voltage as DC. It may have a disable pin to cut power but otherwise needs no interfacing.

  * Supply 3.3 V from a microcontroller and pulse-modulate the output to reach the correct RMS.

* Supply an **AC** voltage.

  * Use a H-bridge (motor controller) that is timed externally. This can be a self-oscillating circuit, a 555 timer or a microcontroller. Hard to get the right timing.

  * Use two GPIO pins and alternate their HIGH/LOW stated on a microcontroller. Maximum draw might be an issue and we're still dealing with too much voltage.

  * Clone the circuitry of a dedicated filament driver with a self-oscillating audio amplifier, capacitor coupling and center-tap biasing with a zener diode. This is Rolo's design but it's pretty large.

### Interfaces

Do I provide a USB-to-Serial adapter on the board? Can I speak to ATTiny84's in this way at all? I suspect you can only program them over ISP anyway. I could probably just spit out the current time over Serial and allow setting the correct time as well.

Buttons? If any, I think I'd use microswitches along one of the edges. I don't want to have anything at all on the front. Switches might come in handy to quickly control things like brightness or date display?

## Links

A collection of useful links and articles:

* [JohnEngineer's ChronodeVFD](https://archive.is/ODOFR): a steampunk-esque wristwatch powered from a 1.5V Alkaline battery ([flickr](https://www.flickr.com/photos/johngineer/15564850491/), [archive]([https://archive.is/ODOFR](https://archive.is/ODOFR))

* [VFD Watch Demystified](https://www.instructables.com/id/Vacuum-Fluorescent-Display-Watch/): a clone of the wristwatch above, complete with schematics and code ([archive](https://archive.is/ojaD5))

* [WiFi Desk Clock](https://www.thingiverse.com/thing:3479496): instructions on using an ESP32 to power the tube and use NTP

* [VFD Clock](http://vwlowen.co.uk/arduino/vfd/vfd-clock.htm): another design, where the filament is driven directly from two Arduino GPIOs, but also contains a description of a regulated H-bridge driver L9110 ([archive](https://archive.is/Xkr9G))

* [Tiny IV-21 VFD Clock](https://hackaday.io/project/167749-tiny-iv-21-vfd-clock#j-discussions-title) uses a hovering actual tube VFD where the filament is powered directly with a voltage divider through a transistor

* [One more build by Kevin Rye](http://kevinrye.net/index_files/vfd_clock_part3.php) which uses a dedicated - albeit obsolete - filament driver: LM9022 (note: this is the same schematic like Rolo's LM4871 driver) ([archive](https://archive.is/C2UBq))

Some fundamental information on VFD operation:

* [Noritake Itron VFD operation guide](https://www.noritake-elec.com/technology/general-technical-information/vfd-operation): contains extensive descriptions of how to drive the filament with center-tapped AC couplings ([archive](https://archive.is/BsdLR))

* [IV-18 Clock](https://web.jfet.org/inGrid/) contains a description of driving anodes with discrete transistors, the author says not to worry about filament AC ([archive](https://archive.is/tNjHp))

* [A series of posts on idyl.io](https://idyl.io/category/vfd/) follows the design of VFD quasi-dot-matrix displays with several different PSU designs based on OpAmps and H-bridges

* Some EEVblog Forum posts:

  * [Designing and OpAmp based power supply for VFD filaments](https://www.eevblog.com/forum/projects/vfd-filament-power-using-lm4871-replacement-for-obsolete-lm9022/)

  * [Datasheet of an old Amp used for that: LM9022M, with example circuits](https://datasheet.octopart.com/LM9022M/NOPB-National-Semiconductor-datasheet-11896945.pdf)

  * [Rolo shows off his universal VFD PSU](https://www.eevblog.com/forum/projects/showing-my-vfd-psu/) based on an LM4871 OpAmp ([archive, no images](https://archive.is/bkgaG))

  * [One particular reply by Ian which details some filament specifics](https://www.eevblog.com/forum/projects/any-interest-in-vfd-tube-power-supply-boards/msg1607869/#msg1607869)

  * [Rolo / r.kamp's pictures folder on some university server?](http://members.casema.nl/r.kamp/pictures/electronics/vfd_psu)
