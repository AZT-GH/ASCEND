# ASCEND
### **Wired Frame Gaming Mouse**

## HARDWARE
The AZT ASCEND is a lightweight usb gaming mouse that is being developed by me in 2025. It is my first usb hid project and my first time using the ATmega32u4. The mouse features three mouse control buttons (no scroll wheel yet) and two smaller buttons for DPI control. A PixArt PMW3389 optical sensor is used for mouse movement and omron d2f-d1f switches are used for all three front buttons. The mouse utilises fullspeed USB2.0 and is compatible with USB C toCc. ESD and overcurrent protection has also been implemented.

### BOM
*Items marked * should not be replaced with generic replacements*\
*Items marked ^ require extra attention to footprints/specifications if they are to be replaced*

| Name                   | Use                          | No. |
|------------------------|------------------------------|-----|
| GT-USB-7010ASV^        | USB Socket                   | 1   |
| SMD1812P020TF          | Resettable Fuse              | 1   |
| SP0502BAHT             | TVS Diodes                   | 1   |
| SMBJ5.0A^              | TVS Diode                    | 1   |
| AMS1117-3.3            | 3.3V LDO                     | 1   |
| AMS1117-1.8            | 1.8V LDO                     | 1   |
| PMW3389DM‐T3QU*        | Optical Sensor               | 1   |
| TXB0108RGY*            | Level Shifter                | 1   |
| ATmega32U4-MU*         | Microcontroller              | 1   |
| AO3400A                | N Channel MOSFET             | 2   |
| XL7EL89COI-111YLC-16M^ | Crystal                      | 1   |
| D2F-01F                | Mouse Switch                 | 3   |
| EVQPLHA15              | Input Switch                 | 2   |
| 5K1 1206 250mW 5%      | Resistor                     |     |
| 10K 1206 250mW 5%      | Resistor                     |     |
| 13R 1206 250mW 5%      | Resistor                     |     |
| 1K 1206 250mW 5%       | Resistor                     |     |
| 22R 1206 250mW 5%      | Resistor                     |     |
| 82R 1206 250mW 5%      | Resistor                     |     |
| 10µF 10V 0805 X7R 10%  | Non Polarised Capacitor      |     |
| 4.7µF 25V 0805 X7R 10% | Non Polarised Capacitor      |     |
| 1µF 50V 0805 X7R 10%   | Non Polarised Capacitor      |     |
| 0.1µF 50V 0805 X7R 10% | Non Polarised Capacitor      |     |
| 12pF 100V 0805 C0G 2%^ | Non Polarised Capacitor      |     |
| TAJA226K010RNJ         | Polarised Tantalum Capacitor | 2   |
| SZYY1206O^             | LED                          | 8   |

## SOFTWARE
The ASCEND features 1ms latency software debouncing for switches. The SPI code and SROM file have been adapted from [PMW3389-DUO](https://github.com/wklenk/pmw3389-duo/tree/main). Many thanks to mrjohnk and wklenk. The debouncing and dpi select code is original.

## SHELL
tbd

## FUTURE
I plan to incorporate a scroll wheel into the design once a fully functioning version of the current design is ready for manufacture. Furthermore, a ui for user adjustment of dpi_array without soldering programming cables is planned. Until then, any mice will have dpi values and polling rate hardcoded onto the mouse (customisable at order time)

I hope that this project helps you with your mouse building endeavour as I found it quite difficult to gather information on this topic.
