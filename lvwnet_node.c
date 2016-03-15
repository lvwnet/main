#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/if_ether.h>
#include <linux/timer.h>
#include <net/mac80211.h>
#include <linux/etherdevice.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/gfp.h>

#define LVWNET_NODE

#include "mac_strtoh.h"
//#include "lvwnet_nl.h"
//#include "lvwnet_timers.h"

#include "lvwnet_proto.h"
//#include "lvwnet_netlink.h"
#include "lvwnet_ethernet.h"
#include "lvwnet_params.h"
#include "lvwnet_sysfs.h"
#include "lvwnet_lfs.h" //loose in free space

//#include "hosts_mesh.h"
//#include "topology/lvwnet_topology.h"
//#include "timers/lvwnet_timers.h"
//#include "nl/lvwnet_nl.h"

//#include "lvwnet.h"

#define LVWNET_VERSION "1.44"

/**
 *  store the hex representation of ctrl_host_addr
 */
unsigned char ctrl_host_addr_h[ETH_ALEN];


static struct net_device *ethernic;
//	static struct net_device *wlannic = NULL;
//static struct net_device *wifi_interf = NULL;
static struct ieee80211_hw* hw;

struct timer_list send_peer_info_timer;
struct timer_list reg_timer;
unsigned int timer_count = 0;

//unsigned int reg_timer_period = 60;
//unsigned int send_peer_info_timer_period = 30;


/**
 * =====================================
 * Begin of function definitions
 * =====================================
 */
static int  __init init_lvwnet(void);
static void __exit exit_lvwnet(void);
static int  __params_verify(void);

static void send_skb(struct sk_buff *skb);
int ethernic_send (struct sk_buff *, uint8_t *,struct net_device*);
static int ethernic_recv (struct sk_buff*, struct net_device*, struct packet_type*, struct net_device*);
void delayed_send (unsigned long int);
static void rcv_hw(struct ieee80211_hw *__hw);
static void __set_ptrs_hw(void);
void send_reg_to_controller(void);
unsigned long nodes_distance(struct lvwnet_node_info*, struct lvwnet_node_info* );
void verify_distance_nodes(void);
void send_data_to_peers(struct sk_buff*);

static spinlock_t lvwnet_lock;
static spinlock_t lvwnet_recv_lock;
static spinlock_t lvwnet_send_reg_lock;

int ethernic_recv_data (struct sk_buff*, struct net_device* , struct packet_type*, struct net_device*);


/**
 * =====================================
 * End of function definitions
 * =====================================
 */

unsigned int DELAYED_SEND_COUNTER = 0;

static struct packet_type pkt_type_lvwnet = {
	.type = htons(LVWNET_ETHERTYPE),
	.func = ethernic_recv, 
};

//static struct packet_type pkt_type_lvwnet_data = {
//	.type = htons(0x0809),
//	.func = ethernic_recv_data, 
//};


//static spinlock_t lvwnet_lock;

/** externs vars from main.c (mac80211) */
extern void (*lvwnet_ptr_hw)(struct ieee80211_hw *hw);
extern void (*lvwnet_ptr_skb)(struct sk_buff *skb);
//extern void (*lvwnet_ptr_skb)(struct sk_buff *skb);
extern void lvwnet_set_loaded(void);
extern void lvwnet_set_loaded(void) { }
extern void lvwnet_set_unloaded(void);
extern void lvwnet_set_unloaded(void) { }


void reg_timer_routine(unsigned long data)
{
    if ((timer_count >= TIMER_SEND_REG) || pos_changed == 1) {
		if (pos_changed) {
			printk(KERN_DEBUG "lvwnet_node: pos changed. sending new reg msg. [%d]\n", timer_count);
		} else {
			printk(KERN_DEBUG "lvwnet_node: sending periodical register to controller... [%d]\n", timer_count);
		}
		timer_count = 0;
		pos_changed = 0;
		
		send_reg_to_controller();
	}
	timer_count++;
    mod_timer(&reg_timer, jiffies + (HZ * 1)); /* restarting timer */
}


static int reg_timer_init(void)
{
    init_timer(&reg_timer);

    reg_timer.function = reg_timer_routine;
    reg_timer.data = 1;
    reg_timer.expires = jiffies + (HZ * 1); /* first time in 1 second, others ... */
    add_timer(&reg_timer); /* Starting the timer */

    printk(KERN_INFO "lvwnet_node: timer loaded... [%s]: %d\n", __func__, __LINE__);
    return 0;
}


/**
 * Set the points for get hw from mac80211 modified module.
 */
static void __set_ptrs_hw(void)
{
    printk(KERN_INFO "lvwnet_node: setting ptrs for real hardware functions.\n");
    lvwnet_ptr_hw = rcv_hw;
}


/**
 * Set send_skb pointer from mac80211 modified module.
 */
static void __set_ptr_skb(void)
{
    printk(KERN_INFO "lvwnet_node: setting ptr for send_skb.\n");
    lvwnet_ptr_skb = send_skb;
}


/**
 * Set the points for get hw from mac80211 modified module.
 */
static void __unset_ptrs_hw(void)
{
    printk(KERN_INFO "lvwnet_node: unsetting ptrs for real hardware functions.\n");
    //spin_lock(&lvwgw_hw_lock);
    lvwnet_ptr_hw = NULL;
    hw = NULL;
    //lvwgw_hw = 0;
    //spin_unlock(&lvwgw_hw_lock);
}


/**
 * send skb via ethernet. TX from mac80211 modified module.
 */
static void send_skb(struct sk_buff *skb)
{
    //struct sk_buff *new_skb;
    //int _headlen = -1;

    if (lvwnet_ptr_skb == NULL) {
        printk(KERN_ALERT "lvwnet_node: ptr_skb is NULL. from [%s]\n", __func__);
        return;
    }
    if (skb == NULL){
        printk(KERN_ALERT "lvwnet_node: skb is NULL. from [%s:%d]\n", __func__, __LINE__);
        return;
    }

	//_headlen = skb_headlen(skb);
    //new_skb = skb_copy(skb, GFP_ATOMIC);

    if (send_all_to_controller == 1) {
		//ethernic_send_msg_type(new_skb, ctrl_host_addr_h, ethernic, 0x07);
		ethernic_send_msg_type(skb, ctrl_host_addr_h, ethernic, 0x07);
	} else {
		//send_data_to_peers(new_skb);
		send_data_to_peers(skb);
	}
}


/**
 * ethernic_recv(...): Recebe e trata pacotes 0x0808 recebidos da rede.
 */
int ethernic_recv (struct sk_buff *skb, struct net_device *dev, struct packet_type *pkt, 
				   struct net_device *orig_dev)
{
	struct sk_buff *skb_recv=NULL;
	//struct sk_buff *skb_recv_to_ieee80211rx=NULL;
	//struct sk_buff *skb_recv_1=NULL;
	//struct sk_buff *newskb=NULL;
	//struct sk_buff *_skb=NULL;
	//struct sk_buff *skb_recv_wlannic=NULL;
	struct ethhdr* eh=NULL;
	//static struct net_device *wifi_interf = NULL;
	//struct lvwnet_reg_omni_header* lh_reg_omni=NULL;
	struct lvwnet_peers_info_header* lh_peers=NULL;
	struct lvwnet_only_flag_header* lh_flag=NULL;
    //struct lvwnet_data_header* lh_data=NULL;
    uint8_t* newdata = NULL;
    //uint8_t* _head = NULL;
    //uint8_t* _data = NULL;
    //uint8_t* _tail = NULL;
    //uint8_t* _end  = NULL;
    //int hdrlen = -1;
    //struct ieee80211_hdr *hdr80211 = NULL;
    //unsigned int _headlen = 0;
	//unsigned char *data1, *data2;    
	int len_data = -1;
	//int len_data_80211 = -1;
	//int len_2 = -1;
    qtd_msg_all++;
    //uint8_t radiotap[26];
    
	/** lock... have or not have... */
	//spin_lock(&lvwnet_lock);


	if( skb_mac_header_was_set(skb) ) {
            eh = eth_hdr(skb);
	} else {
            printk(KERN_ALERT "lvwnet_node: ---- ---- - - skb_mac_header NOT set!\n");
            goto ethernic_recv_out;
	}

	//printk (KERN_INFO "Frame from %pM, to %pM\n", eh->h_source, eh->h_dest);
    //normal node
	if (memcmp( dev->dev_addr, eh->h_dest, ETH_ALEN) != 0 ) {
            printk (KERN_ALERT "lvwnet_node: frame to other host. Device [%s] is in promiscuous mode? to: %pM, from: %pM.\n",
                    dev->name, eh->h_dest, eh->h_source);
            goto ethernic_recv_out;
	}
    //printk(KERN_ALERT "lvwnet_node: size of skb: %ld / %ld\n",sizeof(skb), sizeof(skb_recv));
	skb_recv = skb_copy(skb, GFP_ATOMIC);

	if (skb_recv == NULL){
            printk(KERN_ALERT "lvwnet_node: ERR -> skb_recv(2) == NULL\n");
            goto ethernic_recv_out;
	}

	//skb_reset_network_header(skb_recv);

    if (skb_recv->data == NULL) {
        printk(KERN_ALERT "lvwnet_node: received a NULL skb->data. [%s], line %d\n", __func__, __LINE__);
        goto ethernic_recv_out;
    }


    lh_flag = (struct lvwnet_only_flag_header *) skb_recv->data;
    
	/*************************************************************************/
    if (lh_flag->message_code == LVWNET_CODE_PEER_INFO){
        qtd_msg_peer_info++;
        printk(KERN_DEBUG "lvwnet_node: received a control frame (0x6) from %pM (Peers information).\n", eh->h_source);
        if (is_controller == 1){
            printk(KERN_ALERT "lvwnet_node: received info frame (0x6) but is the controller... [%s]: %d\n", __func__, __LINE__);
            goto ethernic_recv_out;
        }

        lh_peers = (struct lvwnet_peers_info_header *) skb_recv->data;
        lh_peers->delay = ntohs(lh_peers->delay);
        lh_peers->power_rx_dbm = ntohs(lh_peers->power_rx_dbm);

        /**TODO: verificar se o mac do peer eh o do proprio node, e se a origem eh o controller
         * caso sim, atualiza parametros (atualizacao centralizada pelo controller, por exemplo
         * para simular mobilidade conjunta de varios peers).
         */
        //printk(KERN_ALERT "lvwnet_node: %d, %pM\n", lh_peers->message_code, lh_peers->peer_mac);
        peer_received(lh_peers);

        goto ethernic_recv_out;

    }
	/*************************************************************************/
    if (lh_flag->message_code == LVWNET_CODE_REG_OMNI) {
        qtd_msg_reg_omni++;
        printk(KERN_ALERT "lvwnet_node: received a registration frame (0x2) from %pM (Register omni peer).\n", eh->h_source);
		printk(KERN_ALERT "lvwnet_node: received registration frame (0x2) but not controller... [%s]: %d\n", __func__, __LINE__);
		goto ethernic_recv_out;
    }
	/*************************************************************************/
    if (lh_flag->message_code == LVWNET_CODE_DATA) {
        qtd_msg_data++;

		/**TODO: needs this? */
		/*if (wifi_interf == NULL){
			wifi_interf = find_nic("wlan0"); 
			if (wifi_interf == NULL) {
				printk(KERN_ALERT "lvwnet_node: wireless interface (%s) not found! [%s:%d]\n", "wlan0", __func__, __LINE__);
				goto ethernic_recv_out;
			}
		}*/
		
		//skb->dev = wifi_interf;
		
        if (hw == NULL) {
            printk(KERN_ALERT "lvwnet_node: hw is NULL. Wireless NIC is present? (maybe not SoftMAC compatible...) [%s]\n", __func__);
        } else {
			//skb->dev = wifi_interf;
	
			//skb_recv_to_ieee80211rx = skb_copy(skb, GFP_ATOMIC);

			newdata = skb_pull(skb_recv, 1);
			//skb_trim(skb_recv, 2);
			skb_reset_network_header(skb_recv);

			len_data = skb_recv->len;

			if (skb_recv->data_len != 0) {
				printk(KERN_DEBUG "lvwnet_node: data_len != 0. Non-linear skb here... [%s:%d]\n", __func__, __LINE__);
			}
			if (skb_is_nonlinear(skb_recv)) {
				printk(KERN_DEBUG "lvwnet_node: skb_is_nonlinear returned true. Non-linear skb here... [%s:%d]\n", __func__, __LINE__);
			}
			if (skb_recv->data == NULL) {
				printk(KERN_DEBUG "lvwnet_node: skb_recv_to_ieee80211rx->data == NULL. bad... [%s:%d]\n", __func__, __LINE__);
				goto ethernic_recv_out;
			}

			//skb_recv_to_ieee80211rx->csum = skb_checksum_complete(skb_recv_to_ieee80211rx);
			//printk(KERN_DEBUG "lvwnet_node: csum ->[%d, %d] \n", skb_recv->csum, skb_checksum_complete(skb_recv));
			//ieee80211_rx_irqsafe(hw, skb_recv);
			ieee80211_rx(hw, skb_recv);
		 
        } 
        goto ethernic_recv_out;
    } else {
		printk(KERN_ALERT "lvwnet_node: received a unknow message code [%d] \n", lh_flag->message_code);
    }

ethernic_recv_out:
	dev_kfree_skb (skb);
	//spin_unlock(&lvwnet_lock);

	//dev_kfree_skb (skb_recv_to_ieee80211rx);

	return 1;
}



/**
 * ethernic_recv(...): Recebe e trata pacotes 0x0808 recebidos da rede.
 */
int ethernic_recv_data (struct sk_buff *skb, struct net_device *dev, struct packet_type *pkt, 
						struct net_device *orig_dev)
{
	struct sk_buff *skb_recv=NULL;
	struct sk_buff *skb_recv_to_ieee80211rx=NULL;
	//struct sk_buff *skb_recv_1=NULL;
	//struct sk_buff *newskb=NULL;
	//struct sk_buff *_skb=NULL;
	//struct sk_buff *skb_recv_wlannic=NULL;
	struct ethhdr* eh=NULL;
	//static struct net_device *wifi_interf = NULL;
	//struct lvwnet_reg_omni_header* lh_reg_omni=NULL;
	//struct lvwnet_peers_info_header* lh_peers=NULL;
	//struct lvwnet_only_flag_header* lh_flag=NULL;
    //struct lvwnet_data_header* lh_data=NULL;
    //uint8_t* newdata = NULL;
    //uint8_t* _head = NULL;
    //uint8_t* _data = NULL;
    //uint8_t* _tail = NULL;
    //uint8_t* _end  = NULL;
    //int hdrlen = -1;
    //struct ieee80211_hdr *hdr80211 = NULL;
    //unsigned int _headlen = 0;
	//unsigned char *data1, *data2;    
	int len_data = -1;
	//int len_data_80211 = -1;
	//int len_2 = -1;
    
    qtd_msg_all++;
    //uint8_t radiotap[26];
    
	
	spin_lock(&lvwnet_lock);


	if( skb_mac_header_was_set(skb) ) {
            eh = eth_hdr(skb);
	} else {
            printk(KERN_ALERT "lvwnet_node: ---- ---- - - skb_mac_header NOT set!\n");
            goto ethernic_recv_out;
	}

	//printk (KERN_INFO "Frame from %pM, to %pM\n", eh->h_source, eh->h_dest);
    //normal node
	if (memcmp( dev->dev_addr, eh->h_dest, ETH_ALEN) != 0 ) {
            printk (KERN_ALERT "lvwnet_node: frame to other host. Device [%s] is in promiscuous mode? to: %pM, from: %pM.\n",
                    dev->name, eh->h_dest, eh->h_source);
            goto ethernic_recv_out;
	}
    //printk(KERN_ALERT "lvwnet_node: size of skb: %ld / %ld\n",sizeof(skb), sizeof(skb_recv));
	skb_recv = skb_copy(skb, GFP_ATOMIC);

	if (skb_recv == NULL){
            printk(KERN_ALERT "lvwnet_node: ERR -> skb_recv(2) == NULL\n");
            goto ethernic_recv_out;
	}

	//skb_reset_network_header(skb_recv);

    if (skb_recv->data == NULL) {
        printk(KERN_ALERT "lvwnet_node: received a NULL skb->data. [%s], line %d\n", __func__, __LINE__);
        goto ethernic_recv_out;
    }

	qtd_msg_data++;
	

	/*
	 * if (wifi_interf == NULL){
		wifi_interf = find_nic("wlan0");
		if (wifi_interf == NULL) {
			printk(KERN_ALERT "lvwnet_node: wireless interface (%s) not found! [%s:%d]\n", "wlan0", __func__, __LINE__);
			goto ethernic_recv_out;
		}
	}*/

	if (hw == NULL) {
		printk(KERN_ALERT "lvwnet_node: hw is NULL. Wireless NIC is present? (maybe not SoftMAC compatible...) [%s]\n", __func__);
	} else {
		if ( skb == NULL) {
			printk(KERN_ALERT "lvwnet_node: skb is NULL. [%s:%d]\n", __func__, __LINE__);
		} else {
			//skb->dev = wifi_interf;

			skb_recv_to_ieee80211rx = skb_copy(skb, GFP_ATOMIC);

			//skb_recv_1 = skb_pull(skb_recv_to_ieee80211rx, 1);
			skb_reset_network_header(skb_recv_to_ieee80211rx);
		

			
			len_data = skb_recv_to_ieee80211rx->len;

			if (skb_recv_to_ieee80211rx->data_len != 0) {
				printk(KERN_ALERT "lvwnet_node: data_len != 0. Non-linear skb here... [%s:%d]\n", __func__, __LINE__);
			}
			if (skb_is_nonlinear(skb_recv_to_ieee80211rx)) {
				printk(KERN_ALERT "lvwnet_node: skb_is_nonlinear returned true. Non-linear skb here... [%s:%d]\n", __func__, __LINE__);
			}
			if (skb_recv_to_ieee80211rx->data == NULL) {
				printk(KERN_ALERT "lvwnet_node: skb_recv_to_ieee80211rx->data == NULL. bad... [%s:%d]\n", __func__, __LINE__);
				goto ethernic_recv_out;
			}

			if (skb_recv_to_ieee80211rx != NULL){
				if (skb_recv_to_ieee80211rx->head != NULL){
					print_hex_dump(KERN_DEBUG, "0X0808 91(mac_head): ", DUMP_PREFIX_NONE,
									16, 1, (skb_recv_to_ieee80211rx->head + skb_recv_to_ieee80211rx->mac_header), 14, 0);
				}
			}
			printk(KERN_DEBUG ".................................................\n");

			if (skb_recv_to_ieee80211rx != NULL){
				if (skb_recv_to_ieee80211rx->data != NULL){
					print_hex_dump(KERN_DEBUG, "0X0808 92(data    ): ", DUMP_PREFIX_NONE,
								16, 1, skb_recv_to_ieee80211rx->data, len_data, 0);
				}
			}
			printk(KERN_DEBUG ".................................................\n");
			ieee80211_rx_irqsafe(hw, skb_recv_to_ieee80211rx);
		}
	}
	goto ethernic_recv_out;
 
ethernic_recv_out:
	dev_kfree_skb (skb);
	spin_unlock(&lvwnet_lock);

	//dev_kfree_skb (skb_recv_to_ieee80211rx);

	return 1;
}


void send_data_to_peers(struct sk_buff* skb)
{
	//struct sk_buff* _skb;
	//struct sk_buff* _new_data;
	struct sk_buff* _skb_to_send;
    //struct lvwnet_only_flag_header *datahdr;
    //struct lvwnet_data_header *datahdr;
	struct lvwnet_peer_info* temp_peer = NULL;
	
	/** Im the controller, nothing to send... */
	if (is_controller == 1) {
		return;
	}
	
	//printk(KERN_INFO "lvwnet_node: 1 sending data [%*phC], %s:%d\n", _data, __func__, __LINE__);
	//print_hex_dump(KERN_DEBUG, "lvwnet_node:1 raw: ", DUMP_PREFIX_ADDRESS,
	//				16, 1, _data, 64, 1);
    //_new_data = skb_copy(skb, GFP_ATOMIC); /** TODO needs this copy? */

	//return;
    //_skb = alloc_skb(sizeof(struct ethhdr)+sizeof(struct lvwnet_data_header) + 1410, GFP_KERNEL);
    //skb_reserve(_skb, sizeof(struct ethhdr) + sizeof(struct lvwnet_data_header) + 1400);


    //datahdr = (struct lvwnet_data_header*)skb_push(_skb, sizeof(struct lvwnet_data_header));
    //datahdr = (struct lvwnet_only_flag_header*)skb_push(skb, sizeof(struct lvwnet_only_flag_header));

    //skb_reset_network_header(skb); // needs this????? TODO

    //datahdr->message_code = 0x0;
    
    //datahdr->skb_data = skb_copy(_data,GFP_KERNEL);

    if (peers == NULL) {
        printk(KERN_DEBUG "lvwnet_node: nobody to send data... \n");
		goto err_out;
    } else {
        temp_peer = peers;
        
		_skb_to_send = skb_copy(skb,GFP_KERNEL); /** for each node, a new ksb */

        //printk(KERN_INFO "lvwnet_node: sending data to %pM \n", temp_peer->peer_mac);
		//ethernic_send(_skb_to_send,temp_peer->peer_mac,ethernic);
		//ethernic_send_data(_skb_to_send,temp_peer->peer_mac,ethernic);
		ethernic_send_msg_type(_skb_to_send,temp_peer->peer_mac,ethernic, LVWNET_CODE_DATA);

        while (temp_peer->next != NULL){
            temp_peer = temp_peer->next;
			//printk(KERN_INFO "lvwnet_node: sending data to %pM \n", temp_peer->peer_mac);
            _skb_to_send = skb_copy(skb,GFP_KERNEL); /** for each node, a new ksb */
			ethernic_send_msg_type(_skb_to_send,temp_peer->peer_mac,ethernic, LVWNET_CODE_DATA);
			//ethernic_send_data(_skb_to_send,temp_peer->peer_mac,ethernic);

			//ethernic_send(_skb_to_send,temp_peer->peer_mac,ethernic);
        }
    }
	goto err_out;

err_out:

	/** need free somebody here? */
    return;
}

/** Function to periodically send a registration frame message to controller */
void send_reg_to_controller(void)
{
    struct sk_buff* _skb;
    int ret_lh = 0;
	
	spin_lock(&lvwnet_send_reg_lock);
	
    _skb = alloc_skb(sizeof(struct ethhdr)+sizeof(struct lvwnet_reg_omni_header) + 100, GFP_KERNEL);
    skb_reserve(_skb, sizeof(struct ethhdr) + sizeof(struct lvwnet_reg_omni_header));

    //skb_reset_network_header(_skb);

    ret_lh = lvwnet_reg_omni_header_handler (_skb, x_pos, y_pos, z_pos, power_tx_dbm, sens_rx_dbm, channel);
    ethernic_send(_skb,ctrl_host_addr_h,ethernic);
    
	spin_unlock(&lvwnet_send_reg_lock);
    //ethernic_send_msg_type(_skb,ctrl_host_addr_h,ethernic,0x02);
}

/**
 * Receive hw pointer from mac80211 when ieee80211_alloc is called.
 * Is called by mac80211 modified module
 */
static void rcv_hw(struct ieee80211_hw *__hw)
{
    if (__hw == NULL) {
        printk(KERN_ALERT "lvwnet_node: received a NULL hw from mac80211 [%s].\n", __func__);
        goto failed_hw;
    }
    //TODO needs a spin_lock here? 
    if (hw == NULL)
		hw = __hw;
    return;
 failed_hw:
    printk(KERN_ALERT "lvwnet_node: cannot receive hw from mac80211 [%s].\n", __func__);
}


/**
 * __init module function
 */
static int __init init_lvwnet(void)
{
    printk(KERN_INFO "lvwnet_node: Starting module %s now.\n", LVWNET_VERSION);
    if (!__params_verify())
        return -EINVAL; //invalid params

    ethernic = find_nic(ethernic_name);
    if (ethernic == NULL){
        printk(KERN_ALERT "lvwnet_node: ethernet interface [%s] not found.\n", ethernic_name);
        return -EINVAL;
    }
    
	spin_lock_init(&lvwnet_lock);
	spin_lock_init(&lvwnet_recv_lock);
	spin_lock_init(&lvwnet_send_reg_lock);
	
    //Setting pointers to get hw from modified mac80211
    __init_sysfs();
    //extern function from modified mac80211

    /** convert from char to hex */

	__set_ptrs_hw();
	__set_ptr_skb();
	
	lvwnet_set_loaded();

	mac_strtoh(ctrl_host_addr_h, ctrl_host_addr);
	printk(KERN_INFO "lvwnet_node: host set as node, and controller is: [%pM]\n", ctrl_host_addr_h);


    printk(KERN_INFO "lvwnet_node: Registering ethertype 0x0808[lvwnet].\n");
    dev_add_pack(&pkt_type_lvwnet);
    //dev_add_pack(&pkt_type_lvwnet_data);
    printk(KERN_INFO "lvwnet_node: Initializing netlink interface. (not ready yet... sorry...)\n");
	send_reg_to_controller();
	reg_timer_init();
    return 0;
}

/**
 * __exit module function
 */
static void __exit exit_lvwnet(void)
{
    //extern function from modified mac80211
    
    dev_remove_pack(&pkt_type_lvwnet);
    //dev_remove_pack(&pkt_type_lvwnet_data);
    //if (is_controller == 0)
	lvwnet_set_unloaded();
	__unset_ptrs_hw();
	__exit_sysfs();
	del_timer_sync(&reg_timer);

    //if (is_controller == 1)
    //   del_timer_sync(&send_peer_info_timer);
    //clean_timers();
    /** TODO: uses netlink for iw like utility */
    //lvwnet_knetlink_exit();
    printk(KERN_INFO "lvwnet_node: Exiting module now. Version %s.\n", LVWNET_VERSION);
}

module_init(init_lvwnet);
module_exit(exit_lvwnet);

MODULE_DESCRIPTION("LVWNet - Linux Virtual Wireless Network Module");
MODULE_LICENSE("GPL");

