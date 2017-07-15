#/bin/bash!
MODULES_PATH="/lvwnet/modules"
CONFIG_PATH="/lvwnet/config"

MAC_CONTROLLER=$(cat ${CONFIG_PATH}/controller_mac)
NIC_CONTROLLER=$(cat ${CONFIG_PATH}/controller_ethernic_name)
echo "LVWNet controller MAC address: [${MAC_CONTROLLER}] "

insmod  ${MODULES_PATH}/mac80211.ko
insmod  ${MODULES_PATH}/lvwnet_controller.ko ethernic_name=${NIC_CONTROLLER}

