# imAlive

MQTT based watcher for esp-12f which publishes on a topic to indicate liveliness.
The purpose of this project is to set a well known MQTT topic to value *on* as long as IOT device is powered on.
Another app can periodically ping the IOT via MQTT and detect that it is no longer responsive, hopefully because it
has been powered off.

Used with cheap esp8266 from [dx extreme](http://www.cnx-software.com/2015/12/14/3-compact-esp8266-board-includes-rgd-led-photo-resistor-buttons-and-a-usb-to-ttl-interface/).

Reference links:

- [DX Extreme](http://www.dx.com/p/esp-12f-iot-2-4g-wi-fi-wireless-development-board-module-w-antenna-ch340g-driver-black-white-425111#.VsrcpD-vEu5)
- [ESP8266 Witty Cloud Board](https://yoursunny.com/t/2016/WittyCloud-first/)
- [IOT Developer Kit Environment Setup](https://docs.losant.com/getting-started/losant-iot-dev-kits/environment-setup/)

Settings in Arduino IDE for upload to board:

```
Board: ESP-12F IOT + (Generic ESP8266 Module)
SKU: 425111

Generic esp8266 module

1. Flash Mode = QIO
2. Flash Frequency = 40MHz
3. CPU Frequency = 80MHz
4. Flash Size = 4M 1M SPIFFS
5. Reset Method = nodemcu
6. Upload Speed = 115200
```

Pin assignments:

```
const int analogInPin = A0; // Analog input pin that the potentiometer is attached to
const int analogOutPinGREEN = 12; // Analog output pin that the LED is attached to
const int analogOutPinBLUE = 13; // Analog output pin that the LED is attached to
const int analogOutPinRED = 15; // Analog output pin that the LED is attached to
const int buttonPin = 4; // push button
```
