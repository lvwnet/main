/*
 * Linux Virtual Wireless Network Implementation
 * Copyright 2012, Bruno Ferreira <bruno@info.ufrn.br>
 * Copyright 2014, Leonardo Dantas <lodantas@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

spinlock_t lvwnet_lock;

unsigned short int lvwnet_status;

void (*lvwnet_ptr_hw)(struct ieee80211_hw *hw);
EXPORT_SYMBOL_GPL(lvwnet_ptr_hw);

/* init... */
static void lvwnet_init(void)
{
    spin_lock_init(&lvwnet_lock);
    lvwnet_status = 0;
    lvwnet_ptr_hw = NULL;
    lvwnet_ptr_skb = NULL;
    printk(KERN_INFO "[lvwnet]: modified mac80211 started...\n");
}

static void lvwnet_exit(void)
{
    spin_lock_init(&lvwnet_lock);
    lvwnet_status  = 0;
    lvwnet_ptr_hw  = NULL;
    lvwnet_ptr_skb = NULL;
    printk(KERN_INFO "[lvwnet]: exiting from modified mac80211...\n");
}

/* verify if lvwnet is already loaded. 
 * lock safe with spin_lock. 
 */
static int lvwnet_is_loaded(void)
{
    int _return = 0;
    spin_lock(&lvwnet_lock);
 
    if (lvwnet_status) 
         _return = 1;

    spin_unlock(&lvwnet_lock);
    
    return _return;
}
EXPORT_SYMBOL_GPL(lvwnet_is_loaded);

/* set lvwnet as loaded */
static void lvwnet_set_loaded(void)
{
    spin_lock(&lvwnet_lock);
    lvwnet_status = 1;
    spin_unlock(&lvwnet_lock);
}
EXPORT_SYMBOL_GPL(lvwnet_set_loaded);

/* set lvwnet as unloaded */
static void lvwnet_set_unloaded(void)
{
    spin_lock(&lvwnet_lock);
    lvwnet_status = 0;
    spin_unlock(&lvwnet_lock);
}
EXPORT_SYMBOL_GPL(lvwnet_set_unloaded);

static void lvwnet_send_skb_from_mac80211(struct sk_buff *skb)
{
    if (lvwnet_is_loaded()) { 
        if (lvwnet_ptr_skb == NULL) {
            printk(KERN_INFO "[lvwnet]: lvwnet_ptr_skb is NULL! [%s].\n", __func__);
        } else {
            (lvwnet_ptr_skb)(skb);
        }
    } else {
        printk(KERN_INFO "[lvwnet]: lvwnet is unloaded. Nothing to do [%s].\n", __func__);
    }
}

static void lvwnet_send_hw_from_mac80211(struct ieee80211_hw *hw)
{
    if (lvwnet_is_loaded()) {
        if (lvwnet_ptr_hw == NULL) {
            printk(KERN_INFO "[lvwnet]: lvwnet_ptr_hw is NULL! [%s].\n", __func__);
        } else {
            (lvwnet_ptr_hw)(hw);
        }
    } else {
        printk(KERN_INFO "[lvwnet]: lvwnet is unloaded. Nothing to do [%s].\n", __func__);
    }
}

