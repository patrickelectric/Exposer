#!/usr/bin/env bash
root=$PWD
rm $root/simavr/examples/board_simduino/ATmegaBOOT_168_atmega328.ihex
mv $root/Exposer/build/Exposer.ino.hex  $root/simavr/examples/board_simduino/ATmegaBOOT_168_atmega328.ihex

ls
cd simavr/examples/board_simduino
./obj-x86_64-linux-gnu/simduino.elf&
