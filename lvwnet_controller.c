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

#define LVWNET_VERSION "1.44"
#define LVWNET_CONTROLLER
#define LVWNET_TIMER_SEND_INFO 30

//#include "mac_strtoh.h"
//#include "lvwnet_nl.h"
//#include "lvwnet_timers.h"

#include "lvwnet_proto.h"
#include "lvwnet_ethernet.h"
#include "lvwnet_params.h"
#include "lvwnet_sysfs.h"
#include "lvwnet_lfs.h" //loose in free space



static struct net_device *ethernic;

struct timer_list send_peer_info_timer;
struct timer_list reg_timer;
unsigned int timer_count = 0;

unsigned int reg_timer_period = 60;
//unsigned int send_peer_info_timer_period = 30;


/**
 * =====================================
 * Begin of function definitions
 * =====================================
 */
static int  __init init_lvwnet(void);
static void __exit exit_lvwnet(void);
static int  __params_verify(void);

int ethernic_send (struct sk_buff *, uint8_t *,struct net_device*);
static int ethernic_recv (struct sk_buff*, struct net_device*, struct packet_type*, struct net_device*);
unsigned long nodes_distance(struct lvwnet_node_info*, struct lvwnet_node_info* );
void verify_distance_nodes(void);
void send_skb_to_node_peers(uint8_t* , struct sk_buff*);
static spinlock_t lvwnet_lock;

/**
 * =====================================
 * End of function definitions
 * =====================================
 */

static struct packet_type pkt_type_lvwnet = {
	.type = htons(0x0808),
	.func = ethernic_recv, 
};

void send_peer_info_routine(unsigned long data)
{
    verify_distance_nodes();
    mod_timer(&send_peer_info_timer, jiffies + (HZ * LVWNET_TIMER_SEND_INFO)); /* restarting timer */
}

static int send_peer_info_timer_init(void)
{
    init_timer(&send_peer_info_timer);

    send_peer_info_timer.function = send_peer_info_routine;
    send_peer_info_timer.data = 1;
    send_peer_info_timer.expires = jiffies + (HZ * 10); /* first time in 10 seconds, others ... */
    add_timer(&send_peer_info_timer); /* Starting the timer */

    printk(KERN_INFO"lvwnet_ctrl: timer loaded... [%s]: %d\n", __func__, __LINE__);
    return 0;
}


/**
 * ethernic_recv(...): Recebe e trata pacotes 0x0808 recebidos da rede.
 */
int ethernic_recv (struct sk_buff *skb, struct net_device *dev, struct packet_type *pkt, 
				   struct net_device *orig_dev)
{
	struct sk_buff *skb_recv=NULL;
	struct sk_buff *skb_recv_to_ieee80211rx=NULL;
	struct ethhdr* eh=NULL;
	struct lvwnet_reg_omni_header* lh_reg_omni=NULL;
	struct lvwnet_only_flag_header* lh_flag=NULL;

    qtd_msg_all++;

	spin_lock(&lvwnet_lock);


	if( skb_mac_header_was_set(skb) ) {
            eh = eth_hdr(skb);
	} else {
            printk(KERN_ALERT "lvwnet_ctrl: ---- ---- - - skb_mac_header NOT set!\n");
            goto ethernic_recv_out;
	}

    //normal node
	if (memcmp( dev->dev_addr, eh->h_dest, ETH_ALEN) != 0 ) {
            printk (KERN_ALERT "lvwnet_ctrl: frame to other host. Device [%s] is in promiscuous mode? to: %pM, from: %pM.\n",
                    dev->name, eh->h_dest, eh->h_source);
            goto ethernic_recv_out;
	}

	skb_recv = skb_copy(skb, GFP_ATOMIC);
    skb_recv_to_ieee80211rx = skb_copy(skb, GFP_ATOMIC);

	if (skb_recv == NULL){
            printk(KERN_ALERT "lvwnet_ctrl: ERR -> skb_recv(2) == NULL\n");
            goto ethernic_recv_out;
	}

	//skb_reset_network_header(skb_recv);

    if (skb_recv->data == NULL) {
        printk(KERN_ALERT "lvwnet_ctrl: received a NULL skb->data. [%s], line %d\n", __func__, __LINE__);
        goto ethernic_recv_out;
    }


    lh_flag = (struct lvwnet_only_flag_header *) skb_recv->data;

    if (lh_flag->message_code == 0x06){ /** TODO colocar define */
        qtd_msg_peer_info++;
		printk(KERN_ALERT "lvwnet_ctrl: received info frame (0x6) but is the controller... [%s]: %d\n", __func__, __LINE__);
        goto ethernic_recv_out;
    }

    if (lh_flag->message_code == 0x02) {
        qtd_msg_reg_omni++;
        printk(KERN_INFO "lvwnet_ctrl: received a registration frame (0x2) from %pM (Register omni peer).\n", eh->h_source);

        lh_reg_omni = (struct lvwnet_reg_omni_header *) skb_recv->data;

        lh_reg_omni->channel = ntohs(lh_reg_omni->channel);
        lh_reg_omni->pos_x   = ntohl(lh_reg_omni->pos_x);
        lh_reg_omni->pos_y   = ntohl(lh_reg_omni->pos_y);
        lh_reg_omni->pos_z   = ntohl(lh_reg_omni->pos_z);
        lh_reg_omni->power_tx_dbm = ntohs(lh_reg_omni->power_tx_dbm);
        lh_reg_omni->sens_rx_dbm = ntohs(lh_reg_omni->sens_rx_dbm);
        
        node_received(lh_reg_omni,eh->h_source);

        goto ethernic_recv_out;
    }

    if (lh_flag->message_code == LVWNET_CODE_DATA ){
		//verify nodes to send
		send_skb_to_node_peers(eh->h_source,skb_recv);
        goto ethernic_recv_out;
    }

    if (lh_flag->message_code == 0x0) {
        qtd_msg_data++;
		printk(KERN_ALERT "lvwnet_ctrl: received a data frame, but I'm the controller... o.O [%d] \n", lh_flag->message_code);
        goto ethernic_recv_out;
    } else {
		printk(KERN_ALERT "lvwnet_ctrl: received a unknow message code [%d] from %pM \n", lh_flag->message_code, eh->h_source);
        goto ethernic_recv_out;
    }

ethernic_recv_out:
	dev_kfree_skb (skb);
	spin_unlock(&lvwnet_lock);

	return 1;
}

/** Function to periodically send a registration frame message to controller */
void send_info_to_node(struct lvwnet_node_info* node_to_send, struct lvwnet_node_info* peer_of_node,
                       uint16_t power_rx_dbm, uint16_t delay)
{
    struct sk_buff* _skb;
    int ret_lh = 0;

    _skb = alloc_skb(sizeof(struct ethhdr) + sizeof(struct lvwnet_peers_info_header) + 1400, GFP_KERNEL);
    skb_reserve(_skb, sizeof(struct ethhdr) + sizeof(struct lvwnet_peers_info_header));

    skb_reset_network_header(_skb);

    ret_lh = lvwnet_peers_info_header_handler(_skb,peer_of_node->node_mac,power_rx_dbm,delay);

    ethernic_send(_skb,node_to_send->node_mac,ethernic);
}

/** Function to periodically send a info frame message to nodes */
void verify_distance_nodes(void)
{
    struct lvwnet_node_info* next_node = NULL;
    struct lvwnet_node_info* current_node = NULL;
    int32_t distance = -1;
    int16_t lfs = -1;
    int32_t freq = -1;

	/** TODO colocar flag de se existiu alteracao */	

   if (nodes == NULL){
        printk(KERN_ALERT "lvwnet_ctrl: no nodes yet... [%s]: %d\n", __func__, __LINE__);
        return;
    }

    if (nodes->next == NULL) {
        printk(KERN_ALERT "lvwnet_ctrl: only one node yet... [%s]: %d\n", __func__, __LINE__);
        return;
    }  //only 1 node. not to compare...

    current_node = nodes;

    while (current_node->next != NULL){
        next_node = current_node->next;
        distance = nodes_distance(current_node, next_node);

        printk(KERN_INFO "lvwnet_ctrl: distance: %d %pM <-> %pM [%s]:%d\n",
               distance, current_node->node_mac, next_node->node_mac,  __func__, __LINE__);

		/** TODO calcular delay inerente a distância - funcao linear */
		/** TODO: criar uma funcaozinha pra isso... */
		if (current_node->channel == next_node->channel) {
			freq = CHANNEL[current_node->channel];
			lfs = get_lfs_dbm(freq,distance);
			printk(KERN_INFO "lvwnet_ctrl: freq %d, lfs: %d, current_node->power_tx_dbm: %d, next_node->sens_rx_dbm: %d\n ",
					freq, lfs,current_node->power_tx_dbm, next_node->sens_rx_dbm  );

			if ((current_node->power_tx_dbm - lfs) >= next_node->sens_rx_dbm){
				/** TODO: verificar depois se eh isso mesmo (a ordem) */
				send_info_to_node(next_node,current_node,(next_node->power_tx_dbm - lfs),5);
			} else {
				/** TODO seria interessante criar uma lista de peers para serem excluidos, e
				 * mandar em um timer distinto, com menos frequência...
				 */
				send_info_to_node(next_node,current_node,LVWNET_PEER_UNREACHABLE,4);
			}
			if ((next_node->power_tx_dbm - lfs) >= current_node->sens_rx_dbm){
				send_info_to_node(current_node,next_node,(current_node->power_tx_dbm - lfs),3);

			} else {
				/** TODO seria interessante criar uma lista de peers para serem excluidos, e
				 * mandar em um timer distinto, com menos frequência...
				 */
				send_info_to_node(current_node,next_node,LVWNET_PEER_UNREACHABLE,2);
			}

		}
        while (next_node->next != NULL){
            next_node = next_node->next;
            if (memcmp(next_node->node_mac,current_node->node_mac,ETH_ALEN) == 0) { //really need?
                printk(KERN_ALERT "lvwnet_ctrl: identical nodes in list? BUG!!! o.O [%s]: %d\n", __func__, __LINE__);
            }
            distance = nodes_distance(current_node, next_node);
            printk(KERN_INFO "lvwnet_ctrl: distance: %d %pM <-> %pM [%s]:%d\n",
					distance, current_node->node_mac, next_node->node_mac, __func__, __LINE__);
			/** TODO: criar uma funcaozinha pra isso... */
			if (current_node->channel == next_node->channel) {
				freq = CHANNEL[current_node->channel];
				lfs = get_lfs_dbm(freq,distance);
				if ((current_node->power_tx_dbm - lfs) >= next_node->sens_rx_dbm){
					/** TODO: verificar depois se eh isso mesmo (a ordem) */
					send_info_to_node(current_node,next_node,(current_node->power_tx_dbm - lfs),1);
				} else {
					/** TODO seria interessante criar uma lista de peers para serem excluidos, e
					 * mandar em um timer distinto, com menos frequência...
					 */
					send_info_to_node(current_node,next_node,LVWNET_PEER_UNREACHABLE,1);
				}
				if ((next_node->power_tx_dbm - lfs) >= current_node->sens_rx_dbm){
					send_info_to_node(next_node,current_node,(next_node->power_tx_dbm - lfs),1);
				} else {
					/** TODO seria interessante criar uma lista de peers para serem excluidos, e
					 * mandar em um timer distinto, com menos frequência...
					 */
					send_info_to_node(next_node,current_node,LVWNET_PEER_UNREACHABLE,1);
				}

				//send_info_to_node(next_node,current_node,distance,100);

			}
        }
        current_node = current_node->next;
    }
}

/** Used to centralize workflow */
void send_skb_to_node_peers(uint8_t* mac, struct sk_buff* skb)
{
    struct lvwnet_node_info* node1 = NULL;
    struct lvwnet_node_info* node2 = NULL;
    int32_t distance = -1;
    int16_t lfs = -1;
    int32_t freq = -1;


   if (nodes == NULL){
        printk(KERN_ALERT "lvwnet_ctrl: no nodes yet... [%s]: %d\n", 
			__func__, __LINE__);
        return;
    }

    if (nodes->next == NULL) {
        printk(KERN_ALERT "lvwnet_ctrl: only one node yet... [%s:%d]\n", 
			__func__, __LINE__);
        return;
    }  //only 1 node. not to compare...

    node1 = find_node_by_mac(mac);
	if (node1 == NULL){
        printk(KERN_ALERT "lvwnet_ctrl: node with MAC %pM not found [%s:%d]\n", 
			mac, __func__, __LINE__);
		return;
	}

    node2 = nodes;

	while (node2 != NULL){
		if (memcmp(node1->node_mac,node2->node_mac,ETH_ALEN) == 0) {
			node2 = node2->next;
			continue; //the node is the same.
		}
		distance = nodes_distance(node1, node2);
		//printk(KERN_INFO "lvwnet_ctrl: distance: %d %pM <-> %pM [%s]:%d\n",
		//		distance, node1->node_mac, node2->node_mac, __func__, __LINE__);
		if (node1->channel == node2->channel) {
			freq = CHANNEL[node2->channel];
			lfs = get_lfs_dbm(freq,distance);
			if ((node1->power_tx_dbm - lfs) >= node2->sens_rx_dbm){
				//printk(KERN_ALERT "lvwnet_ctrl: broker, sending skb from %pM to %pM [%s:%d]\n", 
				//	node1->node_mac, node2->node_mac, __func__, __LINE__);
				ethernic_send(skb,node2->node_mac,ethernic); //send skb to node
			} 
		} else {
			node2 = node2->next;
			continue; //not the same channel...
		}
		node2 = node2->next;
	}
}



/** return the distance between two nodes (integer) */
unsigned long nodes_distance(struct lvwnet_node_info* node1, struct lvwnet_node_info* node2)
{
    uint32_t mod_x = 0;
    uint32_t mod_y = 0;
    uint32_t mod_z = 0;

    if (node1 == NULL) {
        printk(KERN_ALERT "lvwnet_ctrl: received a NULL lvwnet_node_info (node1). [%s]: %d\n", __func__, __LINE__);
        return -1;
    }
    if (node2 == NULL) {
        printk(KERN_ALERT "lvwnet_ctrl: received a NULL lvwnet_node_info (node2). [%s]: %d\n", __func__, __LINE__);
        return -1;
    }

    mod_x = (node1->pos_x - node2->pos_x) * (node1->pos_x - node2->pos_x);
    mod_y = (node1->pos_y - node2->pos_y) * (node1->pos_y - node2->pos_y);
    mod_z = (node1->pos_z - node2->pos_z) * (node1->pos_z - node2->pos_z);

    return int_sqrt((mod_x + mod_y + mod_z));
}
/**
 * __init module function
 */
static int __init init_lvwnet(void)
{
    printk(KERN_INFO "lvwnet_ctrl: Starting module %s now.\n", LVWNET_VERSION);
    if (!__params_verify())
        return -EINVAL; //invalid params

    ethernic = find_nic(ethernic_name);
    if (ethernic == NULL){
        printk(KERN_ALERT "lvwnet_ctrl: ethernet interface [%s] not found.\n", ethernic_name);
        return -EINVAL;
    }

    //Setting pointers to get hw from modified mac80211
    __init_sysfs();
    //extern function from modified mac80211

	send_peer_info_timer_init();

    printk(KERN_INFO "lvwnet_ctrl: Registering ethertype 0x0808[lvwnet].\n");
    dev_add_pack(&pkt_type_lvwnet);
    printk(KERN_INFO "lvwnet_ctrl: Initializing netlink interface. (not ready yet... sorry...)\n");
    return 0;
}

/**
 * __exit module function
 */
static void __exit exit_lvwnet(void)
{
    //extern function from modified mac80211
    
    dev_remove_pack(&pkt_type_lvwnet);
	del_timer_sync(&send_peer_info_timer);
	__exit_sysfs();
    /** TODO: uses netlink for iw like utility */
    //lvwnet_knetlink_exit();
    printk(KERN_INFO "lvwnet_ctrl: Exiting module now. Version %s.\n", LVWNET_VERSION);
}

module_init(init_lvwnet);
module_exit(exit_lvwnet);

MODULE_DESCRIPTION("LVWNet - Linux Virtual Wireless Network Module - Controller");
MODULE_LICENSE("GPL");

