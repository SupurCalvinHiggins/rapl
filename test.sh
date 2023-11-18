#!/bin/bash
make
insmod hello.ko
cat /sys/kernel/khello/khello
rmmod hello