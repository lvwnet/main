#!/bin/bash
MODULES_PATH="/lvwnet/modules"
CONFIG_PATH="/lvwnet/config"


rmmod  lvwnet_node.ko 
rmmod  lvwnet_controller.ko 
rmmod rt2800usb
rmmod rt2x00usb 
rmmod rt2800lib
rmmod rt2x00lib
rmmod mac80211

