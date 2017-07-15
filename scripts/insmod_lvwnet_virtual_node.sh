#/bin/bash!
MODULES_PATH="/lvwnet/modules"
CONFIG_PATH="/lvwnet/config"

MAC_CONTROLLER=$(cat ${CONFIG_PATH}/controller_mac)
NIC="ens3"
echo "LVWNet controller MAC address: [${MAC_CONTROLLER}] "
X_POS=100
Y_POS=200
Z_POS=300
POWER_TX_DBM=20
SENS_RX_DBM=-75

insmod  ${MODULES_PATH}/lvwnet_node.ko \
	ethernic_name=${NIC} \
	ctrl_host_addr=${MAC_CONTROLLER} \
	x_pos=${X_POS} \
	y_pos=${Y_POS} \
	z_pos=${Z_POS} \
	is_controller=0 \
	power_tx_dbm=${POWER_TX_DBM} \
	sens_rx_dbm=${SENS_RX_DBM} 


#modprobe mac80211_hwsim  radios=1
insmod  ${MODULES_PATH}/mac80211_hwsim.ko radios=1 macaddr= \



