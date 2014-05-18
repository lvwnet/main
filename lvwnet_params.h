#ifndef LVWNET_PARAMS_H_INCLUDED
#define LVWNET_PARAMS_H_INCLUDED

/**
 * =====================================
 * Begin of params definitions
 * =====================================
 */
/**
 * Define if debug is active for lvwnet.
 * In this mode, all __func__ called are printed
 * 0 - disabled
 * 1 - enabled
 */
static int func_debug = 0;
module_param (func_debug, int, 0444);
MODULE_PARM_DESC (func_debug, "Enable debug of lvwnet (all called functions)");


/**
 * Define if debug is active for lvwnet
 * 0 - disabled
 * 1 - enabled
 */
static int debug = 0;
module_param (debug, int, 0444);
MODULE_PARM_DESC (debug, "Enable debug of lvwnet");



/**
 * Define if debug is active for lvwnet
 * 0 - disabled
 * 1 - enabled
 */
static int send_all_to_controller = 0;
module_param (send_all_to_controller, int, 0444);
MODULE_PARM_DESC (send_all_to_controller, 
	"Send all frames to controller, instead of others hosts direct");



/**
 * Define if debug is active for lvwnet
 * 0 - disabled
 * 1 - enabled
 */
static int is_controller = 0;
module_param (is_controller, int, 0444);
MODULE_PARM_DESC (is_controller, "Enable host as controller");

static int x_pos = -1;
module_param (x_pos, int, 0444);
MODULE_PARM_DESC (x_pos, "Define initial X position of node. Meter are the unit of this. ");

static int y_pos = -1;
module_param (y_pos, int, 0444);
MODULE_PARM_DESC (y_pos, "Define initial Y position of node. Meter are the unit of this.");

static int z_pos = -1;
module_param (z_pos, int, 0444);
MODULE_PARM_DESC (z_pos, "Define initial Z position of node. Meter are the unit of this.");

static int power_tx_dbm = 20; //default of 20dBm
module_param (power_tx_dbm, int, 0444);
MODULE_PARM_DESC (power_tx_dbm, "Define initial power of tx in dBm");


static int sens_rx_dbm = -74; //default of -74 dBm
module_param (sens_rx_dbm, int, 0444);
MODULE_PARM_DESC (sens_rx_dbm, "Define initial sensitivity of rx in dBm");

/**
 * Define type of antenna
 * 0 - omni
 * 1 - yagi
 * 2 - sector 90
 * 3 - sector 60
 * 4 - ...
 */
static int antenna_type = 0;
module_param (antenna_type, int, 0444);
MODULE_PARM_DESC (antenna_type, "Define type of antenna");


static int channel = 1;
module_param (channel, int, 0444);
MODULE_PARM_DESC (channel, "Define the channel of transmission (ex.: 1 to 15 in 2.4 GHz and > 100 in 5GHz)");



/**
 * Name of the ethernet interface
 * example: eth0, p1p1 etc
 */
static char* ethernic_name = NULL;
module_param (ethernic_name, charp, 0444);
MODULE_PARM_DESC (ethernic_name, "Name of the ethernet NIC for lvwnet");


/**
 * Mac of the control host
 * must be converted to a hexadecimal representationto be used. This is done by
 * mac_strtoh() function, called at the inicialization of module.
 */
static char* ctrl_host_addr = NULL;
module_param(ctrl_host_addr, charp, 0444);
MODULE_PARM_DESC(ctrl_host_addr, "MAC Address of the host which controls the topology");
/**
 * =====================================
 * End of params definitions
 * =====================================
 */


#ifdef LVWNET_NODE

/**
 * Verify params
 * return 1, ok
 * return 0, bad params
 */
static int __params_verify(void)
{
    /** controller */
    if ( ctrl_host_addr == NULL  && is_controller == 0) {
        printk(KERN_ERR "lvwnet: param ctrl_host_addr was not set, and this node not is the controller...\n");
        return 0;
    }
    if (ethernic_name == NULL) {
        printk(KERN_ERR "lvwnet: param ethernic_name param not set. Please set the ethernet network interface card...\n");
        return 0;
    }
    if (is_controller > 1 || is_controller < 0) {
        printk(KERN_ERR "lvwnet: param is_controller only accepts 0 or 1...\n");
        return 0;
    }
    if ( is_controller == 0 && (x_pos < 0 || y_pos < 0 || z_pos < 0) ) {
        printk(KERN_ERR "lvwnet: params of x, y or z positions are not set...\n");
        return 0;
    }


    return 1;
}
#endif

#ifdef LVWNET_CONTROLLER
/**
 * Verify params
 * return 1, ok
 * return 0, bad params
 */
static int __params_verify(void)
{
    if (ethernic_name == NULL) {
        printk(KERN_ALERT "lvwnet: param ethernic_name param not set. Please set the ethernet network interface card... (ex.:insmod  ethernic_name=eth0)\n");
        return 0;
    }
    return 1;
}
#endif

#endif // LVWNET_PARAMS_H_INCLUDED
