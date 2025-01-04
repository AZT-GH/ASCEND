# ASCEND

HARDWARE
The AZT ASCEND is a lightweight usb gaming mouse that is being developed in 2025. It is my first usb hid project and my first time using the ATMega32u4. The mouse features three mouse control buttons (no scroll wheel yet) and two smaller buttons for dpi control. A pixart pmw3389 optical sensor is used for mouse movement and omron d2f-d1f are used for all three front buttons. The mouse utilises fullspeed usb2.0 and is compatible with usb c to c. ESD and overcurrent protection has also been implemented.

SOFTWARE
The ASCEND features zero latency software debouncing for the 3 mouse control buttons and standard 50ms software deboucning for the dpi buttons. The SPI code and SROM file have been sourced from https://github.com/wklenk/pmw3389-duo/tree/main. The debouncing and dpi select code is original.

FUTURE
I plan to incorporate a scroll wheel into the design once a fully functioning version of the current design is ready for manufacture. Furthermore, a ui for user adjustment of dpi_array without soldering programming cables is planned. Until then, any sold mice will have dpi values and polling rate hardcoded onto the mouse (customisable at order time)

I hope that this project helps you with your mouse building endeavour as I found it quite difficult to gather information on this topic.
