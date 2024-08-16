This repo is intended for a Arduino R4 Wifi used to read measurements from a ADIS16480 IMU and Ublox MAX-M10S (Sparkfun breakout). 

The Max-M10S is connected with a Qwicc cable for I2C communication and has a jumper going from the PPS port to Pin3, triggering interrupts on PPS signals.

The ADIS device is communicating over SPI with CS at pin 7. Power (3.3V and GND) is connected to the Arduino board. PIN2 is connected to the DIO2 on the ADIS, and is used as a data ready signal, triggering an interrupt that saves the timestamp and reads IMU data. 

The Ethernet Shield 2 is also used to send all received data to a host computer. This is handled by the "network" module.


NB!!!!! To make this work properly, you need to overwrite the legacy SPI library. Add this folder: https://github.com/oveggeland/arduino_r4_spi to the libraries directory for your arduino IDE. 