#!/bin/sh

if [ ! -c /dev/pstate_user ]  ; then
    echo "Creating a dev file..."
    sudo mknod -m 666 /dev/pstate_user c 210 0
    exit 0
fi


sudo sh -c "echo noboost > /sys/devices/system/cpu/turbofreq/pstate_policy"

