#!/bin/sh

if [ ! -c /dev/pstate_user ]  ; then
    sudo mknod -m 666 /dev/pstate_user c 210 0
    echo "Created /dev/pstate_user"
fi

# setting noboost to switch to manual control by the intercoolr API
sudo sh -c "echo noboost > /sys/devices/system/cpu/turbofreq/pstate_policy"



