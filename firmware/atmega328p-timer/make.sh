#!/bin/sh

set -e

avr-gcc -std=c11 -mmcu=atmega328p -Os main.c -o main.o
avr-objcopy -j .text -j .data -O ihex main.o main.hex
avr-objcopy -j .eeprom --change-section-lma .eeprom=0 -O ihex main.o main.eeprom

echo "Done!"
