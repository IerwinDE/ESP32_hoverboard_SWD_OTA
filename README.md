The goal of this software is to connect a ESP32 to the Mainboards that can be found in Hoverboards and flash them over the air.
For more info on Hoverboard hacking and the firmware that enables so many great projects, check out: https://github.com/EFeru/hoverboard-firmware-hack-FOC
This is my way of giving something back to the awesome community that has built around this hardware.

The whole ARM SWD implementation has been done ofer at https://github.com/scanlime/esp8266-arm-swd.
I had to fork that repo for now so it can be compiled for ESP32 but hope that the changes will be pulles at some point.

The code is in a very early PoC stage, so use at your own risk.

But it flashes and it makes things with wheels go fast, so YEY!


//TODO:
- prevent the boards from turning off while connected due to timeout or voltage check
- Make it possible to have the ESP check a Server for available updates on each start and do hands-off-update (Pull vs push)
- make proper headers in all sourcecode files
- add a Link so people can buy me a coffee :P
