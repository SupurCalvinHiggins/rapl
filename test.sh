#!/bin/bash
make
insmod rcal.ko
cat /sys/kernel/rcal/rcal_calibrate
rmmod rcal