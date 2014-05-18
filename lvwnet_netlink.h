
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#ifndef LVNET_NETLINK_H
#define LVNET_NETLINK_H

#define NETLINK_USER 31

static struct lvwnconf_command {
	char action[3];
	char mac_address[12];
	int coord_x;
	int coord_y;
	int power;
};

static void lvwnet_nl_recv_msg(struct sk_buff *skb);
void substr(char *origem,char *destino,int inicio,int comprimento);

struct sock *nl_sk = NULL; //socket Netlink
struct netlink_kernel_cfg cfg = {
    .input = lvwnet_nl_recv_msg,
};

unsigned int preq_ms = 5, prep_ms = 12, perror_ms = 17;
char *payload;
uint8_t *p;
char msg[350] = ""; // Msg que sera enviada ao usuario


static int lvwnet_knetlink_init(void)
{
	//struct lvwnconf_command lcomm;
    printk("Iniciando: %s\n",__FUNCTION__);

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);//Cria o socket netlink kernel

    if(!nl_sk)
    {
        printk(KERN_ALERT "Erro ao criar socket.\n");
        return -10;
    }
    return 0;
}

static void lvwnet_knetlink_exit(void){
    printk(KERN_INFO "Fechando Kernel Netlink\n");
    netlink_kernel_release(nl_sk);
}


static void lvwnet_nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;		//Cabeçalho Netlink
    int pid;					// PID do processo do usuario
    struct sk_buff *skb_out;	// Buffer da Mensagem netlink
    int msg_size; 				// Tamanho da Msg
    int res, tamanho;			// Se der erro no envio da msg res < 0

    printk(KERN_INFO "Entrando em: %s\n", __FUNCTION__);

    nlh=(struct nlmsghdr*)skb->data; // Pega a msg do usuario
    p = NLMSG_DATA(nlh);


    printk(KERN_INFO "Msg Netlink recebida do usuario: \n");

	tamanho = *(uint16_t *)p;
	printk(KERN_INFO "Tamanho: %d \n", tamanho);

	//printk(KERN_INFO "received netlink message payload:%s\n", NLMSG_DATA(nlh));

	payload = p + 2;
	printk(KERN_INFO "p2: %s \n", payload);

	// Agora retorna mensagem para processo usuario

	pid = nlh->nlmsg_pid; /*pid do usuario */
	msg_size = strlen(msg); //Tamanho da msg que sera enviada
	printk(KERN_INFO "Msg: %s, Tamanho: %d\n", msg, msg_size);// Imprime o tamanho da msg q será enviada
	skb_out = nlmsg_new(msg_size,0); // Cria novo buffer para enviar a mensagem
	if(!skb_out) {
		printk(KERN_ERR "Falha ao alocar novo buffer socket\n");
		return;
	}
	//Adiciona uma nova msg netlink (struct sk_buff *skb, u32 portid, u32 seq, int type, int payload, int flags)
	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	NETLINK_CB(skb_out).dst_group = 0; /* n tem grupo multcast */
	strncpy(nlmsg_data(nlh), msg, msg_size);
	//memcpy(NLMSG_DATA(nlh), msg, msg_size);
	res = nlmsg_unicast(nl_sk, skb_out, pid);
	if(res<0)
		printk(KERN_INFO "Erro enquanto enviava msg para o usuario\n");
}


#endif	// LVWNET_NL_H
