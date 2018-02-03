#!/bin/sh

set -e

./make.sh
avrdude -P /dev/ttyUSB0 -b 19200 -c avrisp -p atmega328p -v -e -U flash:w:main.hex
avrdude -P /dev/ttyUSB0 -b 19200 -c avrisp -p atmega328p -v -U eeprom:w:main.eeprom
