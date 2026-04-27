# BenchATX

[![Firmware Build](https://github.com/Quphoria/BenchATX/actions/workflows/firmware_build.yml/badge.svg)](https://github.com/Quphoria/BenchATX/actions/workflows/firmware_build.yml)

Bench power supply using an ATX PSU with current sensors  

![PCB Render](<ATX PSU Bench Supply.png>)

[Schematic](./ATX%20PSU%20Bench%20Supply%20Schematic.pdf)

## Possible Improvements

- Move OLED up 2.54mm so 1.3" OLEDs won't obstruct the current shunt voltage headers
- Add solder jumpers to swap OLED GND/VCC
- Add solder jumpers to swap OLED between i2c0 and i2c1
- Add a temperature sensor
