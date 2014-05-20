#ifndef LVWNET_PROTO_H_INCLUDED
#define LVWNET_PROTO_H_INCLUDED

#include <linux/delay.h>

#define LVWNET_CODE_PEER_INFO    0x06
#define LVWNET_CODE_REG_OMNI     0x02
#define LVWNET_CODE_DATA         0x07
#define LVWNET_PEER_UNREACHABLE -256
#define LVWNET_ETHERTYPE         0x0808

/** Time to send registration to controller from peers */
#define TIMER_SEND_REG 	         120 
/** Time to send registration to controller from peers */
#define TIMER_SEND_INFO_NODES    60 
//#define TIMER_SEND_REG_CTRL      30 

spinlock_t  lvwnet_list_lock;
int lock_inited = 0;

int pos_changed = 0;

long qtd_msg_peer_info = 0;
long qtd_msg_reg_omni  = 0;
long qtd_msg_data      = 0;
long qtd_msg_all       = 0;


/** header of lvwnet peers information */
struct lvwnet_peers_info_header
{
    uint8_t message_code; //1 byte
    unsigned char peer_mac[ETH_ALEN]; //6 bytes
    int16_t power_rx_dbm; //2 bytes
    uint16_t delay; //2 bytes - in ms
} __attribute__ ((packed));

/** header of lvwnet peers information */
struct lvwnet_data_header
{
    uint8_t message_code; //1 byte
    struct sk_buff* skb_data;  // e o tamanho??? usa char?? TODO 
} __attribute__ ((packed));

/** struct of lvwnet peers information */
struct lvwnet_peer_info
{
    unsigned char peer_mac[ETH_ALEN];
    int16_t power_rx_dbm;
    uint16_t delay;
    struct lvwnet_peer_info* prev;
    struct lvwnet_peer_info* next;
};

/** header of lvwnet register omni data */
struct lvwnet_reg_omni_header
{
    uint8_t message_code;    //1 byte
    uint32_t pos_x;          //4 bytes
    uint32_t pos_y;          //4 bytes
    uint32_t pos_z;          //4 bytes
    int16_t power_tx_dbm; //2 bytes
    int16_t sens_rx_dbm;  //2 bytes
    uint16_t channel;        //2 bytes
} __attribute__ ((packed));


/** struct of lvwnet node information - use only by controller*/
struct lvwnet_node_info
{
    unsigned char node_mac[ETH_ALEN];
    uint32_t pos_x;
    uint32_t pos_y;
    uint32_t pos_z;
    int16_t power_tx_dbm;
    int16_t sens_rx_dbm;
    uint16_t channel;
    struct lvwnet_node_info* prev;
    struct lvwnet_node_info* next;
};

/** header of lvwnet with only first flag*/
struct lvwnet_only_flag_header
{
    uint8_t message_code; //1 byte
} __attribute__ ((packed));/** linked list of all peers */



void print_peers(struct lvwnet_peer_info* );/** linked list of all peers */

void __peer_add(struct lvwnet_peer_info*);
void __peer_remove(struct lvwnet_peer_info*);
void __peer_unreachable_add(struct lvwnet_peer_info*);
void __peer_unreachable_remove(struct lvwnet_peer_info*);
void __node_add(struct lvwnet_node_info*);

struct lvwnet_peer_info* find_peer_by_mac(const void*, struct lvwnet_peer_info*);
struct lvwnet_node_info* find_node_by_mac(const void*);


/** linked list of all peers */
struct lvwnet_peer_info* peers = NULL;

/** linked list of all nodes */
struct lvwnet_node_info* nodes = NULL;

struct lvwnet_peer_info* peers_unreachables = NULL;

void list_lock(void)
{
    if (lock_inited == 0 ){
        lock_inited = 1;
        spin_lock_init(&lvwnet_list_lock);
        spin_lock(&lvwnet_list_lock);
    }
    else
    {
        spin_lock(&lvwnet_list_lock);
    }
    //spin_lock_init(lvwnet_list_lock);
    //spin_lock(lvwnet_list_lock);

}

void list_unlock(void)
{
    spin_unlock(&lvwnet_list_lock);
}

/** manipulate node in a linked list - wrapper */
void node_received(struct lvwnet_reg_omni_header* lh, const void* _node_mac)
{
    struct lvwnet_node_info* node_temp;
    struct lvwnet_node_info* node_found;

    list_lock();
    if (lh == NULL || _node_mac == NULL) {
        printk(KERN_ALERT "lvwnet: received a NULL lvwnet_reg_omni_header or node mac. [%s], line %d\n", __func__, __LINE__);
        goto node_received_out;
    }

    node_temp = kmalloc(sizeof(struct lvwnet_node_info), GFP_KERNEL);

    node_temp->next = NULL;
    node_temp->prev = NULL;

    node_temp->pos_x          = lh->pos_x;
    node_temp->pos_y          = lh->pos_y;
    node_temp->pos_z          = lh->pos_z;
    node_temp->power_tx_dbm   = lh->power_tx_dbm;
    node_temp->sens_rx_dbm    = lh->sens_rx_dbm;
    node_temp->channel        = lh->channel;
    memcpy(node_temp->node_mac, _node_mac,ETH_ALEN); //mac...

    node_found = find_node_by_mac(_node_mac);

    if ( node_found == NULL) {  //node not found...
        __node_add(node_temp);
    } else {    //node found
        printk(KERN_INFO "lvwnet: node %pM need update. \n", node_found->node_mac);

        node_found->pos_x          = node_temp->pos_x;
        node_found->pos_y          = node_temp->pos_y;
        node_found->pos_z          = node_temp->pos_z;
        node_found->power_tx_dbm = node_temp->power_tx_dbm;
        node_found->sens_rx_dbm  = node_temp->sens_rx_dbm;
        node_found->channel        = node_temp->channel;
    }

node_received_out:

    list_unlock();
    return;
}


/** manipulate peer in a linked list - wrapper */
void peer_received(struct lvwnet_peers_info_header* lh)
{
    struct lvwnet_peer_info* peer_temp = NULL;
    struct lvwnet_peer_info* peer_found = NULL;

    list_lock();
    if (lh == NULL) {
        printk(KERN_ALERT "lvwnet: received a NULL lvwnet_peers_info_header. [%s], line %d\n", __func__, __LINE__);
        goto peer_add_out;
    }


    peer_temp = kmalloc(sizeof(struct lvwnet_peer_info), GFP_KERNEL);

    memcpy(peer_temp->peer_mac, lh->peer_mac,ETH_ALEN); //mac...
    peer_temp->delay =  lh->delay;
    peer_temp->power_rx_dbm =  lh->power_rx_dbm;
    peer_temp->next = NULL;
    peer_temp->prev = NULL;


    peer_found = find_peer_by_mac(lh->peer_mac, peers);

    //__peer_add(peer_temp);

    if ( peer_found == NULL) {
        if (peer_temp->power_rx_dbm <= LVWNET_PEER_UNREACHABLE) {
            printk(KERN_INFO "lvwnet: peer %pM is unreachable and not in list. Not to do. \n", peer_temp->peer_mac);
            if (find_peer_by_mac(lh->peer_mac, peers_unreachables) == NULL) {
                __peer_unreachable_add(peer_temp);
            }
            goto peer_add_out;
        }
		if (find_peer_by_mac(lh->peer_mac, peers_unreachables) != NULL) {
			__peer_unreachable_remove(peer_temp);
		}
        __peer_add(peer_temp);
    } else {
        if (peer_temp->power_rx_dbm <= LVWNET_PEER_UNREACHABLE){
            printk(KERN_INFO "lvwnet: peer %pM need be removed. \n", peer_found->peer_mac);
            __peer_remove(peer_found);
            if (find_peer_by_mac(lh->peer_mac, peers_unreachables) == NULL){
                __peer_unreachable_add(peer_temp);
            }
            goto peer_add_out;
        } else { //nao precisa desse else...
            printk(KERN_INFO "lvwnet: peer %pM need update. \n", peer_found->peer_mac);
        }

		if (find_peer_by_mac(lh->peer_mac, peers_unreachables) != NULL) {
			__peer_unreachable_remove(peer_temp);
		}

        peer_found->delay = peer_temp->delay;
        peer_found->power_rx_dbm = peer_temp->power_rx_dbm;
    }
peer_add_out:
    //print_peers(peer_temp);

    list_unlock();

    return;
}


/** add node in a linked list */
void __node_add(struct lvwnet_node_info* _node)
{
    int count = 0;
    struct lvwnet_node_info* temp_node = NULL;

    printk(KERN_ALERT "lvwnet: called [%s], line %d\n", __func__, __LINE__);

    if (_node == NULL)
    {
        printk(KERN_ALERT "lvwnet: received a NULL lvwnet_node_info. [%s], line %d\n", __func__, __LINE__);
        return;
    }

    if (nodes == NULL)   //first time...
    {
        nodes = _node;
        printk(KERN_ALERT "lvwnet: first node. [%s], line %d\n", __func__, __LINE__);
    }
    else
    {
        temp_node = nodes;

        while (temp_node->next != NULL)
        {
            temp_node = temp_node->next;
            count++;
        }

        //printk(KERN_ALERT "lvwnet: return 5 [%d]\n", count);
        _node->prev = temp_node;
        temp_node->next = _node;
    }
}




/** add peer in a linked list */
void __peer_add(struct lvwnet_peer_info* _peer)
{
    int count = 0;
    struct lvwnet_peer_info* temp_peer = NULL;

    //printk(KERN_ALERT "lvwnet: called [%s], line %d\n", __func__, __LINE__);
    if (_peer == NULL){
        printk(KERN_ALERT "lvwnet: received a NULL lvwnet_peer_info. [%s:%d]\n", __func__, __LINE__);
        return;
    }
    if (peers == NULL) {//first time
        peers = _peer;
        printk(KERN_INFO "lvwnet: first peer. [%s:%d]\n", __func__, __LINE__);
    } else {
        temp_peer = peers;
        //printk(KERN_ALERT "lvwnet: return 3\n");
        while (temp_peer->next != NULL) {
            temp_peer = temp_peer->next;
            count++;
        }

        //printk(KERN_ALERT "lvwnet: return 5 [%d]\n", count);
        _peer->prev = temp_peer;
        temp_peer->next = _peer;
    }
}


/** remove peer from linked list */
void __peer_remove(struct lvwnet_peer_info* _peer)
{
    if (_peer->prev == NULL && _peer->next == NULL)   //last of list
    {
        printk(KERN_ALERT "lvwnet: last peer of list. [%s], line %d\n", __func__, __LINE__);
        peers = NULL;
        return;
    }

    if (_peer->prev == NULL)   //head of list
    {
        printk(KERN_ALERT "lvwnet: peer is the head of list. [%s], line %d\n", __func__, __LINE__);

        peers = peers->next;
        peers->prev = NULL;
        return;
    }

    if (_peer->next == NULL)   //tail of list
    {
        printk(KERN_ALERT "lvwnet: peer is the tail of list. [%s], line %d\n", __func__, __LINE__);
        _peer->prev->next = NULL;
        return;
    }
    //midle of list
    _peer->prev->next = _peer->next;
    _peer = NULL;
}


/** TODO: generalize */
void __peer_unreachable_add(struct lvwnet_peer_info* _peer)
{
    int count = 0;
    struct lvwnet_peer_info* temp_peer = NULL;

    //printk(KERN_ALERT "lvwnet: called [%s], line %d\n", __func__, __LINE__);
    if (_peer == NULL)
    {
        printk(KERN_ALERT "lvwnet: received a NULL lvwnet_peer_info. [%s:%d]\n", __func__, __LINE__);
        return;
    }
    if (peers_unreachables == NULL)   //first time...
    {
        peers_unreachables = _peer;
        printk(KERN_INFO "lvwnet: first peer. [%s:%d]\n", __func__, __LINE__);
    }
    else
    {
        temp_peer = peers_unreachables;
        //printk(KERN_ALERT "lvwnet: return 3\n");
        while (temp_peer->next != NULL)
        {
            temp_peer = temp_peer->next;
            count++;
        }

        //printk(KERN_ALERT "lvwnet: return 5 [%d]\n", count);
        _peer->prev = temp_peer;
        temp_peer->next = _peer;
    }
}

/** TODO: generalize */
/** remove peer from linked list */
void __peer_unreachable_remove(struct lvwnet_peer_info* _peer)
{
    if (_peer->prev == NULL && _peer->next == NULL)   //last of list
    {
        printk(KERN_ALERT "lvwnet: last peer of list. [%s], line %d\n", __func__, __LINE__);
        peers_unreachables = NULL;
        return;
    }

    if (_peer->prev == NULL)   //head of list
    {
        printk(KERN_ALERT "lvwnet: peer is the head of list. [%s], line %d\n", __func__, __LINE__);

        peers_unreachables = peers_unreachables->next;
        peers_unreachables->prev = NULL;
        return;
    }

    if (_peer->next == NULL)   //tail of list
    {
        printk(KERN_ALERT "lvwnet: peer is the tail of list. [%s], line %d\n", __func__, __LINE__);
        _peer->prev->next = NULL;
        return;
    }
    //midle of list
    _peer->prev->next = _peer->next;
    _peer = NULL;
}

/** find a peer by mac address*/
struct lvwnet_peer_info* find_peer_by_mac(const void* _mac,  struct lvwnet_peer_info* _peer_list)
{
    struct lvwnet_peer_info* temp_peer = NULL;

    if (_mac == NULL){
        printk(KERN_ALERT "lvwnet: received a NULL mac address. [%s], line %d\n", __func__, __LINE__);
        return NULL;
    }
    if (_peer_list == NULL){
        return NULL;
    }

    if (memcmp(_mac,_peer_list->peer_mac,ETH_ALEN) == 0) {
        return _peer_list;
    }
    temp_peer = _peer_list;
    while (temp_peer->next != NULL){
        temp_peer = temp_peer->next;
        if (memcmp(_mac,temp_peer->peer_mac,ETH_ALEN) == 0){
            return temp_peer;
        }
    }
    return NULL;
}

/** TODO generalizar */
/** find a node by mac address*/
struct lvwnet_node_info* find_node_by_mac(const void* _mac)
{
    struct lvwnet_node_info* temp_node = NULL;

    if (_mac == NULL) {
        printk(KERN_ALERT "lvwnet: received a NULL mac address. [%s], line %d\n", __func__, __LINE__);
        return NULL;
    }
    if (nodes == NULL) {
       // printk(KERN_ALERT "lvwnet: peers list empty yet. [%s], line %d\n", __func__, __LINE__);
        return NULL;
    }

    if (memcmp(_mac,nodes->node_mac,ETH_ALEN) == 0)
    {
        return nodes;
    }
    temp_node = nodes;
    while (temp_node->next != NULL)
    {
        temp_node = temp_node->next;
        if (memcmp(_mac,temp_node->node_mac,ETH_ALEN) == 0)
        {
            return temp_node;
        }
    }
    return NULL;
}

/** used for manipulate lvwnet_peers_info_header */
size_t lvwnet_peers_info_header_handler(struct sk_buff *skb, const void *peer_mac,
                                        uint16_t power_rx_dbm, uint16_t delay)
{
    struct lvwnet_peers_info_header *peerhdr =
        (struct lvwnet_peers_info_header *)skb_push(skb, sizeof(struct lvwnet_peers_info_header));

    if (!peer_mac)
        goto err_out;
    else
        memcpy(peerhdr->peer_mac, peer_mac, ETH_ALEN);

    peerhdr->message_code = 6;
    //peerhdr->message_code = LVWNET_CODE_PEER_INFO;
    peerhdr->power_rx_dbm = htons(power_rx_dbm);
    peerhdr->delay = htons(delay);

    return  sizeof(struct lvwnet_peers_info_header);

err_out:
    return -sizeof(struct lvwnet_peers_info_header);
}

/** used for manipulate lvwnet_peers_info_header */
size_t lvwnet_data_header_handler(struct sk_buff *skb, struct sk_buff* skb_data, int size)
{
    struct lvwnet_data_header *peerhdr =
        (struct lvwnet_data_header *)skb_push(skb, sizeof(struct lvwnet_data_header));

    if (skb_data == NULL)
        goto err_out;
    else
    
    memcpy(peerhdr->skb_data, skb_data, size);
	peerhdr->message_code = 0;
    //peerhdr->message_code = LVWNET_CODE_PEER_INFO;
    return sizeof(struct lvwnet_data_header);

err_out:
    return -sizeof(struct lvwnet_data_header);
}


void print_peers(struct lvwnet_peer_info* _peer)
{
    printk(KERN_INFO "---------------------------------------------------------------------\n");

    if (_peer == NULL)
    {
        printk(KERN_ALERT "lvwnet: received a NULL peer. [%s], line %d\n", __func__, __LINE__);
        return;
    }
    if (_peer->prev != NULL)
    {
        printk(KERN_INFO "lvwnet: previous peer mac: %pM\n", _peer->prev->peer_mac);
    }
    else
    {
        printk(KERN_INFO "lvwnet: previous peer mac: NULL\n");
    }
    printk(KERN_INFO "lvwnet: current  peer mac: %pM\n", _peer->peer_mac);
    printk(KERN_INFO "lvwnet: current  delay   : %d\n" , _peer->delay);
    printk(KERN_INFO "lvwnet: current  power_rx: %d\n", _peer->power_rx_dbm);

    if (_peer->next != NULL)
    {
        printk(KERN_INFO "lvwnet: next     peer mac: %pM\n", _peer->next->peer_mac);
    }
    else
    {
        printk(KERN_INFO "lvwnet: next     peer mac: NULL\n");
    }


}
/** used for manipulate lvwnet_reg_omni_header */
int lvwnet_reg_omni_header_handler(struct sk_buff *skb,
                                   uint32_t pos_x, uint32_t pos_y, uint32_t pos_z,
                                   uint16_t power_tx_dbm, uint16_t sens_rx_dbm,
                                   uint16_t channel)
{
    struct lvwnet_reg_omni_header *reghdr =
        (struct lvwnet_reg_omni_header *)skb_push(skb, sizeof(struct lvwnet_reg_omni_header));

    /*
     * htonxx:
     * On the i386 the host byte order is Least Significant Byte first,
     * whereas the network byte order, as used on the Internet, is Most
     * Significant Byte first.

     * only have sense in uint16 or above
     * (order of less or most significant byte)
    */

    reghdr->message_code = 0x02;
    //reghdr->message_code = LVWNET_CODE_REG_OMNI;
    reghdr->pos_x = htonl(pos_x); //4 bytes long
    reghdr->pos_y = htonl(pos_y); //4 bytes long
    reghdr->pos_z = htonl(pos_z); //4 bytes long
    reghdr->power_tx_dbm = htons(power_tx_dbm); //2 bytes long
    reghdr->sens_rx_dbm = htons(sens_rx_dbm); //2 bytes long
    reghdr->channel = htons(channel); //2 bytes long

    return sizeof(struct lvwnet_reg_omni_header);
}

#endif
