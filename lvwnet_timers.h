/*
 * Teste de funcao de timer do Linux
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/list.h>

#ifndef LVWNET_TIMERS_H
#define LVWNET_TIMERS_H

MODULE_LICENSE("GPL");

/*
 * basic node for timers list
 */
struct lvwnet_timer {
	struct timer_list* tl;
	struct list_head lt_head;
};

LIST_HEAD (list_of_timers);

//int counter;

/*
void lvwnet_delayed_send (unsigned long data)
{
	struct timer_params* tp = (struct timer_params*) data;
	printk (KERN_ALERT "Timer Callback called (%ld).\n", jiffies);
	printk (KERN_ALERT "tp->i=%d; tp->str=%s.\n", tp->i, tp->str);
	counter++;
}
*/

int do_in_timer (void* function, unsigned long int params, unsigned int mseconds) {

	int ret = 0;
	struct lvwnet_timer* new_timer;

	new_timer = kmalloc( sizeof(struct lvwnet_timer), GFP_KERNEL );

	// Adiciona timer na lista de timers
	new_timer->tl = (struct timer_list*) kmalloc( sizeof(struct timer_list), GFP_KERNEL );
	list_add_tail( &(new_timer->lt_head), &(list_of_timers) );

	// Configura o novo timer
	//setup_timer (&newtimer, test_timer_callback, (unsigned long int) tp[i]);
	//setup_timer (new_timer->tl, function, (unsigned long int) tp[i]);
	setup_timer (new_timer->tl, function, params);

	// Aciona o novo timer
	//printk (KERN_ALERT "Starting timer to fire in 1s (%ld)\n", jiffies);
	ret = mod_timer (new_timer->tl, jiffies + msecs_to_jiffies(mseconds));
	//if (ret) printk (KERN_ALERT "Error in mod_timer\n");

	return ret;		
}

/*
int init_module (void)
{
	int i;

	// criar vetor de ponteiros para timer_params...
	struct timer_params* tp[10];

	printk (KERN_ALERT "Test Timer Module loaded\n");
	counter=0;
	for (i=0;i<10;i++) {
		tp[i] = kmalloc (sizeof(struct timer_params), GFP_KERNEL);
		tp[i]->i = i;
		tp[i]->str = kmalloc (10, GFP_KERNEL);
		memcpy (tp[i]->str, "123456789\0", 10);

		in_timer (&lvwnet_delayed_send, (unsigned long int) tp[i], 1000);
	}

	return 0;
}
*/

int clean_fired_timers (void)
{
	int count=0;
	struct lvwnet_timer *tmer, *tmp_tmer;
	struct list_head* pos;

	// !!! TODO: APAGAR TAMBEM ELEMENTOS DA LISTA ( list_del() ) !!! 
	list_for_each_entry_safe (tmer, tmp_tmer, &list_of_timers, lt_head) {
		//tmp = list_entry (pos, struct lvwnet_timer, lt_head);
		if ( timer_pending(tmer->tl) ) {
			del_timer (tmer->tl);
			list_del (&tmer->lt_head);
			kfree (tmer);
			count++;
		}
	}
	return count;
}

void clean_timers (void)
{
	int count=0;
	struct lvwnet_timer *tmer, *tmp_tmer;
	//struct lvwnet_timer* tmp;
	struct list_head* pos;

	// !!! TODO: APAGAR TAMBEM ELEMENTOS DA LISTA ( list_del() ) !!! 
	// ver: http://www.roman10.net/linux-kernel-programminglinked-list/
	// usar list_for_each_entry_safe(pos, n, head, member)
	//list_for_each(pos, &list_of_timers){
	list_for_each_entry_safe (tmer, tmp_tmer, &list_of_timers, lt_head) {
		//tmp = list_entry (pos, struct lvwnet_timer, lt_head);
		del_timer (tmer->tl);
		list_del (&tmer->lt_head);
		kfree (tmer);
		count++;
	}

	printk (KERN_ALERT "%i timers deleted.\n", count);
	printk (KERN_ALERT "test_timer module unloading\n");

	return;
}

#endif	// LVWNET_TIMERS_H
