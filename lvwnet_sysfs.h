#ifndef LVWNET_SYSFS_H_INCLUDED
#define LVWNET_SYSFS_H_INCLUDED
#include "lvwnet_proto.h"

static int  __init_sysfs(void);
static void __exit_sysfs(void);


/**
 * sysfs objects
 */
static struct kobject *sysfs_lvwnet;

/** Show fw version of real wireless nic */
static ssize_t sysfs_hw_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    /*if (hw == NULL) {
        return sprintf(buf,"%s\n", "real wireless nic not loaded");
    } else {
        if (strcmp(attr->attr.name, "fw_version" ) == 0) {
            return sprintf(buf,"%s\n", hw->wiphy->fw_version);
        } else if (strcmp(attr->attr.name, "perm_addr" ) == 0) {
            return sprintf(buf,"%s\n", hw->wiphy->perm_addr);
            //return sprintf(buf,"%s\n", hw->wiphy->addresses->addr);
            //return sprintf(buf,"%s\n", hw->wiphy->dev->init_name);
        } else {
            return sprintf(buf,"%s\n", "attribute not found");
        }
    }*/
    return sprintf(buf,"%s\n", "TODO...");
}

/** Show fw version of real wireless nic */
static ssize_t sysfs_perm_addr(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf,"%s\n", "TODO...");
}

/** Show fw version of real wireless nic */
static ssize_t sysfs_qtd_peers(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int count = 0;
    struct lvwnet_peer_info* temp_peer = NULL;

    if (peers == NULL) {
        return 0;
    } else {
        temp_peer = peers;
        count++;

        while (temp_peer->next != NULL){
            temp_peer = temp_peer->next;
            count++;
        }
    }
    return sprintf(buf,"%d\n", count);
}

/** Show fw version of real wireless nic */
static ssize_t sysfs_qtd_nodes(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int count = 0;
    struct lvwnet_node_info* temp_node = NULL;

    if (nodes == NULL) {
        return 0;
    } else {
        temp_node = nodes;
        count++;
        while (temp_node->next != NULL){
            temp_node = temp_node->next;
            count++;
        }
    }
    return sprintf(buf,"%d\n", count);
}

/** Show fw version of real wireless nic */
static ssize_t sysfs_oper_mode(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    if (is_controller == 0) {
        return sprintf(buf,"%s\n", "node");
    } else {
        return sprintf(buf,"%s\n", "controller");
    }
}

static ssize_t sysfs_peers_list(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    struct lvwnet_peer_info* temp_peer = NULL;
    int _str_size = 0;
    if (peers == NULL) {
        return sprintf(buf,"%s\n", "no peers yet.");
    } else {
        temp_peer = peers;
        //sprintf(buf,"%pM, \n", temp_node->node_mac);
        _str_size = sprintf(buf + _str_size,"%pM, power_rx=%ddBm, delay=%dms \n",
                            temp_peer->peer_mac, temp_peer->power_rx_dbm, temp_peer->delay);

        while (temp_peer->next != NULL){
            temp_peer = temp_peer->next;
            _str_size += sprintf(buf + _str_size,"%pM, power_rx=%ddBm, delay=%dms \n",
                                temp_peer->peer_mac, temp_peer->power_rx_dbm, temp_peer->delay);

        }
    }
    return strlen(buf);
}

static ssize_t sysfs_peers_unreachable_list(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    struct lvwnet_peer_info* temp_peer = NULL;
    int _str_size = 0;
    if (peers_unreachables == NULL) {
        return sprintf(buf,"%s\n", "no peers yet.");
    } else {
        temp_peer = peers_unreachables;
        //sprintf(buf,"%pM, \n", temp_node->node_mac);
        _str_size = sprintf(buf + _str_size,"%pM, power_rx=%ddBm, delay=%dms \n",
                            temp_peer->peer_mac, temp_peer->power_rx_dbm, temp_peer->delay);

        while (temp_peer->next != NULL){
            temp_peer = temp_peer->next;
            _str_size += sprintf(buf + _str_size,"%pM, power_rx=%ddBm, delay=%dms \n",
                                temp_peer->peer_mac, temp_peer->power_rx_dbm, temp_peer->delay);

        }
    }
    return strlen(buf);
}

static ssize_t sysfs_nodes_list(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    struct lvwnet_node_info* temp_node = NULL;
    int _str_size = 0;
    if (nodes == NULL) {
        return sprintf(buf,"%s\n", "no nodes yet.");
    } else {
        temp_node = nodes;
        //sprintf(buf,"%pM, \n", temp_node->node_mac);
        _str_size = sprintf(buf,"%pM, x/y/z=%d/%d/%d, power_tx=%ddBm, sens_rx=%ddBm, channel=%d \n",
                    temp_node->node_mac, temp_node->pos_x,temp_node->pos_y,temp_node->pos_z,
                    temp_node->power_tx_dbm, temp_node->sens_rx_dbm, temp_node->channel);

        while (temp_node->next != NULL){
            temp_node = temp_node->next;
            _str_size += sprintf(buf + _str_size,"%pM, x/y/z=%d/%d/%d, power_tx=%ddBm, sens_rx=%ddBm, channel=%d \n",
                    temp_node->node_mac, temp_node->pos_x,temp_node->pos_y,temp_node->pos_z,
                    temp_node->power_tx_dbm, temp_node->sens_rx_dbm, temp_node->channel);

        }
    }
    return strlen(buf);
}

/** Show fw version of real wireless nic */
static ssize_t sysfs_nodes_gnuplot(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    struct lvwnet_node_info* temp_node = NULL;
    int _str_size = 0;
    if (nodes == NULL) {
        return sprintf(buf,"%s\n", "no nodes yet.");
    } else {
        temp_node = nodes;
        //sprintf(buf,"%pM, \n", temp_node->node_mac);
        _str_size = sprintf(buf,"%d %d %d\n",
                    temp_node->pos_x,temp_node->pos_y,temp_node->pos_z);

        while (temp_node->next != NULL) {
            temp_node = temp_node->next;
            _str_size += sprintf(buf + _str_size,"%d %d %d\n",
                    temp_node->pos_x,temp_node->pos_y,temp_node->pos_z);
        }
    }
    return strlen(buf);
}

static ssize_t sysfs_qtd_all_msg(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf,"%ld\n", qtd_msg_all);
}

static ssize_t sysfs_qtd_data_msg(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf,"%ld\n", qtd_msg_data);
}

static ssize_t sysfs_qtd_info_msg(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf,"%ld\n", qtd_msg_peer_info);
}

static ssize_t sysfs_qtd_reg_omni_msg(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf,"%ld\n", qtd_msg_reg_omni);
}

static ssize_t sysfs_x_pos_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf,"%d\n", x_pos);
}

static ssize_t sysfs_x_pos_store(struct kobject *kobj, struct kobj_attribute *attr, 
	const char *buf, size_t count)
{
    sscanf(buf,"%d", &x_pos);
    return count;
}

static ssize_t sysfs_y_pos_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf,"%d\n", y_pos);
} 

static ssize_t sysfs_y_pos_store(struct kobject *kobj, struct kobj_attribute *attr, 
	const char *buf, size_t count)
{
    sscanf(buf,"%d", &y_pos);
    return count;
}

static ssize_t sysfs_z_pos_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf,"%d\n", z_pos);
}

static ssize_t sysfs_z_pos_store(struct kobject *kobj, struct kobj_attribute *attr, 
	const char *buf, size_t count)
{
    sscanf(buf,"%d", &z_pos);
	pos_changed = 1;
    return count;
}

static struct kobj_attribute fw_attribute = __ATTR(fw_version, 0666, sysfs_hw_show, NULL);
static struct kobj_attribute perm_addr_attribute = __ATTR(perm_addr, 0666, sysfs_perm_addr, NULL);
static struct kobj_attribute qtd_peers = __ATTR(qtd_peers, 0666, sysfs_qtd_peers, NULL);
static struct kobj_attribute qtd_nodes = __ATTR(qtd_nodes, 0666, sysfs_qtd_nodes, NULL);
static struct kobj_attribute oper_mode = __ATTR(oper_mode, 0666, sysfs_oper_mode, NULL);
static struct kobj_attribute peers_list = __ATTR(peers_list, 0666, sysfs_peers_list, NULL);
static struct kobj_attribute peers_unreachable_list = __ATTR(peers_unreachable_list, 0666, sysfs_peers_unreachable_list, NULL);
static struct kobj_attribute nodes_list = __ATTR(nodes_list, 0666, sysfs_nodes_list, NULL);
static struct kobj_attribute nodes_gnuplot = __ATTR(nodes_gnuplot, 0666, sysfs_nodes_gnuplot, NULL);
static struct kobj_attribute qtd_data_msg = __ATTR(qtd_data_msg, 0666, sysfs_qtd_data_msg, NULL);
static struct kobj_attribute qtd_all_msg = __ATTR(qtd_all_msg, 0666, sysfs_qtd_all_msg, NULL);
static struct kobj_attribute qtd_info_msg = __ATTR(qtd_info_msg, 0666, sysfs_qtd_info_msg, NULL);
static struct kobj_attribute qtd_reg_omni_msg = __ATTR(qtd_reg_omni_msg, 0666, sysfs_qtd_reg_omni_msg, NULL);
static struct kobj_attribute x_pos_kobj = __ATTR(x_pos, 0666, sysfs_x_pos_show, sysfs_x_pos_store);
static struct kobj_attribute y_pos_kobj = __ATTR(y_pos, 0666, sysfs_y_pos_show, sysfs_y_pos_store);
static struct kobj_attribute z_pos_kobj = __ATTR(z_pos, 0666, sysfs_z_pos_show, sysfs_z_pos_store);


/** 
 * TODO: colocar sysfs para:
 * qtd de peers
 * qtd de nodes
 * modo de operacao
 * lista de todos os peers
 * lista de todos os nodes
 * qtd de mensagens recebidas por tipo (reg, info e data)
 */


static struct attribute *attrs[] = {
    &fw_attribute.attr,
    &perm_addr_attribute.attr,
    &qtd_peers.attr,
    &qtd_nodes.attr,
    &oper_mode.attr,
    &peers_list.attr,
    &peers_unreachable_list.attr,
    &nodes_list.attr,
    &nodes_gnuplot.attr,
    &qtd_all_msg.attr,
    &qtd_data_msg.attr,
    &qtd_info_msg.attr,
    &qtd_reg_omni_msg.attr,
    &x_pos_kobj.attr,
    &y_pos_kobj.attr,
    &z_pos_kobj.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

/**
 * Init kobjects for sysfs lvwnet
 */
static int __init_sysfs(void)
{
    int __ret;
    sysfs_lvwnet = kobject_create_and_add("lvwnet", kernel_kobj);
    if (!sysfs_lvwnet)
        return -ENOMEM;
    /* Create files associated with this kobject */
    __ret = sysfs_create_group(sysfs_lvwnet, &attr_group);
    if (__ret)
        kobject_put(sysfs_lvwnet);
    return __ret;
}
/**
 * Unload sysfs for lvwnet
 */
static void __exit_sysfs(void)
{
    printk(KERN_INFO "lvwnet: cleaning sysfs entry.\n");
    kobject_put(sysfs_lvwnet);
}



#endif // LVWNET_SYSFS_H_INCLUDED
