# High Altitude Balloon
High Altitude Balloon for the Global Space Balloon Challenge

## Controller
The primary microcontroller used is a [Teensy 3.5](https://www.pjrc.com/store/teensy35.html). The Teensy is a Arduino-compatible ARM based development board. 

Software will be based on the [Trackuino project](https://github.com/trackuino/trackuino) with modifications for Teensy DAC support from [this project.](https://github.com/rvnash/aprs_teensy31)

## Communications
Telemetry will be sent via 2m APRS using the [HX1 2m Transmitter](http://www.radiometrix.com/content/hx1). This module has an output power of ~300mW tuned to a frequency of 144.390 MHz. There are reports of frequency drift with temperature, so this behavior will have to be characterized before launch.

Antenna will be a simple half-wavelength dipole, likely based on [this](http://bear.sbszoo.com/construction/antenna/cf/hwcf2.htm) or [this.](http://bear.sbszoo.com/construction/antenna/cf/hwcf1.htm)

### HX-1 Transmitter
- [Main Page](http://www.radiometrix.com/content/hx1)

Pinout

Pin Label | Functionality | Wire Color
--------- | ------------- | ----------
1, 3 | RF Ground | N/A
2 | RF out | N/A 
4 | Active-high Enable | N/A
5 | 5V Supply | N/A
6 | GND | N/A
7 | TX Data | N/A

# Sensors
## U-Blox MAX-M8 GPS Receiver
### Datasheets
- [Breakout Board](https://store.uputronics.com/?route=product/product&product_id=72)
- [Hardware Specs](https://www.u-blox.com/sites/default/files/MAX-M8-FW3_DataSheet_%28UBX-15031506%29.pdf)
- [Protocol Spec](https://www.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_%28UBX-13003221%29_Public.pdf)

### Software
Outputs NEMA strings over serial, so should integrate with Trackuino's code nicely. Might have to do some configuration via UART.

Config example: http://ava.upuaut.net/?p=738

### I/O
Pin Label | Functionality | Wire Color
--------- | ------------- | ----------
SDA | I2C | N/A
SCL | I2C | N/A 
TXD | Serial UART | Blue
RXD | Serial UART | Yellow
GND | Ground | Black
VCC | 3.3V Power | Red

## BMP180 Barometer/Temperature
### Datasheets
- [Adafruit](https://www.adafruit.com/products/1603)
- [Library](https://learn.sparkfun.com/tutorials/bmp180-barometric-pressure-sensor-hookup-/installing-the-arduino-library)

### Software
Use the Sparkfun library. Communicates over I2C, nothing fancy. Must use SCL/SDA0 since those correspond to the default Wire interface.

### I/O
Pin Label | Functionality | Wire Color
--------- | ------------- | ----------
VIN | 5V power in, feeds on-board regulator | N/A
3V3 | 3V3 power rail | N/A 
GND | Ground | Black
SCL | I2C | SCL0 (Pin 19)
SDA | I2C | SDA0 (Pin 18)

## MMA8451 3-axis accelerometer
### Datasheets
- [Adafruit](https://www.adafruit.com/product/2019)
- [Library](https://learn.adafruit.com/adafruit-mma8451-accelerometer-breakout/wiring-and-test)

### Software
Use the Adafruit library. Communicates over I2C.

### I/O
Pin Label | Functionality | Wire Color
--------- | ------------- | ----------
VIN | 5V power in, feeds on-board regulator | N/A
3Vo | 3V3 power rail | N/A 
GND | Ground | Black
SCL | I2C | N/A
SDA | I2C | N/A
A | Address Select | NC for address 0x1C
I1 | Interrupt 1 | NC
I2 | Interrupt 2 | NC

## 3950 NTC Thermristor
### Datasheets
- [Adafruit](https://www.adafruit.com/product/372)
- [Tutorial](https://learn.adafruit.com/thermistor/using-a-thermistor)

Hook it up with a 10k resistor in a voltage divider setup, read with ADC.

# Actuators

## Electric heating pad
