#!/bin/sh

driverModule="unDriver"
driverName="myDriver"
deviceName="myDevice"

modeDevice="a+w"

rmmod "${driverModule}".ko

insmod "${driverModule}".ko myMajor=0

rm -f /dev/"${deviceName}"[0-3]

majorNumber=$(awk '$2~/myDriver/ {print $1}' /proc/devices)

mknod /dev/"${deviceName}"0 c $majorNumber 0
mknod /dev/"${deviceName}"1 c $majorNumber 1
mknod /dev/"${deviceName}"2 c $majorNumber 2
mknod /dev/"${deviceName}"3 c $majorNumber 3

chmod "${modeDevice}" /dev/"${deviceName}"[0-3]

