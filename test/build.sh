#!/bin/sh
g++ -std=c++17 -I../src -ggdb3 ../src/Logger.cpp main.cpp -lpthread -o test
objcopy -O binary --only-section=.rodata test test.rodata