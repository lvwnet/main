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

void (*lvwnet_ptr_skb)(struct sk_buff *skb);
EXPORT_SYMBOL_GPL(lvwnet_ptr_skb);
void (*lvwnet_ptr_hw)(struct ieee80211_hw *hw);
EXPORT_SYMBOL_GPL(lvwnet_ptr_hw);

