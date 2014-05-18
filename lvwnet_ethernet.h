#ifndef LVWNET_ETHERNET_H_INCLUDED
#define LVWNET_ETHERNET_H_INCLUDED

spinlock_t lvwnet_send_lock;

int lvwnet_send_lock_flag = 0;
/**
 * find_nic(): Procura por uma determinada interface de rede no sistema.
 *                  Recebe: string contendo o nome da interface.
 *                  Retorna: Ponteiro para o dispositivo encontrado ou NULL.
 */
static struct net_device* find_nic(char *ethname){
     struct net_device *dev;
     printk (KERN_INFO "lvwnet: find_nic called. Interface name: [%s]\n", ethname);
     dev = first_net_device(&init_net);
     while (dev) {
             if( memcmp(dev->name, ethname, 4) == 0 ){
                     return(dev);
             }
             dev = next_net_device(dev);
     }
     return NULL;
}

	

/**
 * ethernic_send(): Envia pacote pela interface real de rede.
 * Recebe: estrutura sk_buff e endereco MAC de destino.
 * Retorna: void.
 */
int ethernic_send (struct sk_buff *skb, uint8_t *dest_mac, struct net_device* _dev)
{
    int ret=0;
    //int ret_eh = 0;
	uint8_t * new_skb_data;
	struct sk_buff* newskb = skb;
	static uint8_t protocol_code[] = "\x08\x08";
	//uint8_t msg_type = 0x07;

	if (lvwnet_send_lock_flag == 0) {
		lvwnet_send_lock_flag = 1;
		spin_lock_init(&lvwnet_send_lock);
	}
	spin_lock(&lvwnet_send_lock);

	if (dest_mac == NULL) {
		printk(KERN_ALERT "lvwnet: error, received null mac address! [%s:%d]\n", __func__, __LINE__);
        goto ethernic_send_out;
	}
	if (_dev == NULL) {
		printk(KERN_ALERT "lvwnet: error, received null net_device! [%s:%d]\n", __func__, __LINE__);
        goto ethernic_send_out;
	}
	if (skb == NULL) {
		printk(KERN_ALERT "lvwnet: error, received null skb_buffer! [%s:%d]\n", __func__, __LINE__);
        goto ethernic_send_out;
	}

    if(_dev->flags & IFF_UP) {
        skb->dev = _dev;
        newskb->dev = _dev;
        /* We need to check if the extended area for the new skb is
         * greater than the headroom, to avoid a kernel panic.
         */

        if (skb_headroom(skb) < ETH_HLEN){
            printk(KERN_ALERT "lvwnet: no space left on sk_buffer headroom. Expanding it... [%s:%d]\n", __func__, __LINE__);
            newskb = skb_copy_expand (skb, ETH_HLEN+8, 0, GFP_ATOMIC); /**TODO: why +8??? */
        }
        
        new_skb_data = skb_push(newskb, ETH_HLEN);

        memcpy(new_skb_data, dest_mac, ETH_ALEN);
        memcpy(new_skb_data+ETH_ALEN, _dev->dev_addr,  ETH_ALEN);
        memcpy(new_skb_data+(2*ETH_ALEN), protocol_code, 2);
        //memcpy(new_skb_data+(2*ETH_ALEN)+2, &msg_type, 1);

        //ret_eh = eth_header(newskb, _dev, 0x0808, dest_mac,_dev->dev_addr,ETH_HLEN);
        //newskb->dev = _dev;
	
		//printk(KERN_INFO "lvwnet: 3 head [%d], data [%d], tail [%d], end [%d], %s:%d\n", 
		//			&skb->head, &skb->data, &skb->tail, &skb->end, 14, __func__, __LINE__);

		//print_hex_dump(KERN_DEBUG, "lvwnet:3 raw: ", DUMP_PREFIX_ADDRESS,
		//		16, 1, skb, skb->len, 1);

		skb_reset_network_header(newskb);
		//skb_reset_mac_header(newskb);

        ret = dev_queue_xmit (newskb);

        if (ret < 0)
            printk (KERN_ALERT "lvwnet: failed to send frame. [%s:%d]\n", __func__, __LINE__);

        goto ethernic_send_out;

    }

ethernic_send_out:
	spin_unlock(&lvwnet_send_lock);
	return 0;
}


/**
 * ethernic_send(): Envia pacote pela interface real de rede.
 * Recebe: estrutura sk_buff e endereco MAC de destino.
 * Retorna: void.
 */
int ethernic_send_data (struct sk_buff *skb, uint8_t *dest_mac, struct net_device* _dev)
{
    int ret=0;
	uint8_t * new_skb_data;
	struct sk_buff* newskb = skb;
	static uint8_t protocol_code[] = "\x08\x09";

	if (lvwnet_send_lock_flag == 0) {
		lvwnet_send_lock_flag = 1;
		spin_lock_init(&lvwnet_send_lock);
	}
	spin_lock(&lvwnet_send_lock);


	if (dest_mac == NULL) {
		printk(KERN_ALERT "lvwnet: error, received null mac address! [%s:%d]\n", __func__, __LINE__);
        goto ethernic_send_out;
	}
	if (_dev == NULL) {
		printk(KERN_ALERT "lvwnet: error, received null net_device! [%s:%d]\n", __func__, __LINE__);
        goto ethernic_send_out;
	}
	if (skb == NULL) {
		printk(KERN_ALERT "lvwnet: error, received null skb_buffer! [%s:%d]\n", __func__, __LINE__);
        goto ethernic_send_out;
	}

    if(_dev->flags & IFF_UP) {
        skb->dev = _dev;
        newskb->dev = _dev;
        /* We need to check if the extended area for the new skb is
         * greater than the headroom, to avoid a kernel panic.
         */

        if (skb_headroom(newskb) < ETH_HLEN){
            printk(KERN_ALERT "lvwnet: no space left on sk_buffer headroom. Expanding it... [%s:%d]\n", __func__, __LINE__);
            newskb = skb_copy_expand (newskb, ETH_HLEN+8, 0, GFP_ATOMIC); /**TODO: why +8??? */
        }
        
        new_skb_data = skb_push(newskb, ETH_HLEN);

        memcpy(new_skb_data, dest_mac, ETH_ALEN);
        memcpy(new_skb_data+ETH_ALEN, _dev->dev_addr,  ETH_ALEN);
        memcpy(new_skb_data+(2*ETH_ALEN), protocol_code, 2);


		skb_reset_network_header(newskb);

        //if (skb_headroom(newskb) < ETH_HLEN){
        //    printk(KERN_ALERT "lvwnet: no space left on sk_buffer headroom. Expanding it... [%s:%d]\n", __func__, __LINE__);
        //    newskb = skb_copy_expand (newskb, ETH_HLEN+8, 0, GFP_ATOMIC); /**TODO: why +8??? */
        //}
		
		//TODO TIRAR!!!!!!!!!!!!!!!!! Gambiarra!!!!!!!!!!! Testando!!!!!!!!!!!!!
		
        //new_skb_data = skb_push(newskb, ETH_HLEN);

        //memcpy(new_skb_data, dest_mac, ETH_ALEN);
        //memcpy(new_skb_data+ETH_ALEN, _dev->dev_addr,  ETH_ALEN);
        //memcpy(new_skb_data+(2*ETH_ALEN), protocol_code, 2);

		//skb_reset_mac_header(newskb);

        ret = dev_queue_xmit (newskb);
        goto ethernic_send_out;

        if (ret < 0)
            printk (KERN_ALERT "lvwnet: failed to send frame. [%s:%d]\n", __func__, __LINE__);
    }

ethernic_send_out:
	spin_unlock(&lvwnet_send_lock);
	return 0;
}



/**
 * ethernic_send(): Envia pacote pela interface real de rede.
 * Recebe: estrutura sk_buff e endereco MAC de destino.
 * Retorna: void.
 */
int ethernic_send_msg_type (struct sk_buff *skb, uint8_t *dest_mac, 
                            struct net_device* _dev, uint8_t msg_type)
{
    int ret=0;
    //int ret_eh = 0;
	uint8_t * new_skb_data;
	struct sk_buff* newskb = skb;
	static uint8_t protocol_code[] = "\x08\x08"; //TODO colocar #define
	//uint8_t msg_type = 0x07;

	if (lvwnet_send_lock_flag == 0) {
		lvwnet_send_lock_flag = 1;
		spin_lock_init(&lvwnet_send_lock);
	}
	spin_lock(&lvwnet_send_lock);


	if (dest_mac == NULL) {
		printk(KERN_ALERT "lvwnet: error, received null mac address! [%s:%d]\n", __func__, __LINE__);
        goto ethernic_send_out;
	}
	if (_dev == NULL) {
		printk(KERN_ALERT "lvwnet: error, received null net_device! [%s:%d]\n", __func__, __LINE__);
        goto ethernic_send_out;
	}
	if (skb == NULL) {
		printk(KERN_ALERT "lvwnet: error, received null skb_buffer! [%s:%d]\n", __func__, __LINE__);
        goto ethernic_send_out;
	}

    if(_dev->flags & IFF_UP) {
        skb->dev = _dev;
        newskb->dev = _dev;
        
        /* We need to check if the extended area for the new skb is
         * greater than the headroom, to avoid a kernel panic.
         */
        if (skb_headroom(skb) < (ETH_HLEN + 1)){
            printk(KERN_ALERT "lvwnet: no space left on sk_buffer headroom. Expanding it... [%s:%d]\n", __func__, __LINE__);
            newskb = skb_copy_expand (skb, ETH_HLEN+8, 0, GFP_ATOMIC); /**TODO: why +8??? parece que o radiotap é 8... */
        }
        
        new_skb_data = skb_push(newskb, ETH_HLEN+1);


        memcpy(new_skb_data, dest_mac, ETH_ALEN);
        memcpy(new_skb_data+ETH_ALEN, _dev->dev_addr,  ETH_ALEN);
        memcpy(new_skb_data+(2*ETH_ALEN), protocol_code, 2);
        memcpy(new_skb_data+(2*ETH_ALEN)+2, &msg_type, 1);


        //ret_eh = eth_header(newskb, _dev, 0x0808, dest_mac,_dev->dev_addr,ETH_HLEN);
        //newskb->dev = _dev;
	
		//printk(KERN_INFO "lvwnet: 3 head [%d], data [%d], tail [%d], end [%d], %s:%d\n", 
		//			&skb->head, &skb->data, &skb->tail, &skb->end, 14, __func__, __LINE__);

		//print_hex_dump(KERN_DEBUG, "lvwnet:3 raw: ", DUMP_PREFIX_ADDRESS,
		//		16, 1, skb, skb->len, 1);

		skb_reset_network_header(newskb);
		//skb_reset_mac_header(newskb);

        ret = dev_queue_xmit (newskb);
        goto ethernic_send_out;

        if (ret < 0)
            printk (KERN_ALERT "lvwnet: failed to send frame. [%s:%d]\n", __func__, __LINE__);

    }

ethernic_send_out:
	spin_unlock(&lvwnet_send_lock);

	return 0;
}

#endif // LVWNET_ETHERNET_H_INCLUDED
