#/bin/bash!
MODULES_PATH="/lvwnet/modules"

insmod  ${MODULES_PATH}/mac80211.ko
modprobe rt2x00lib
modprobe rt2800lib
modprobe rt2x00usb 
modprobe rt2800usb


