#include "mac.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

mac_port_map_t mac_port_map;

// initialize mac_port table     FDB表？  HASH_8BITS==256
void init_mac_port_table()
{
	bzero(&mac_port_map, sizeof(mac_port_map_t));

	for (int i = 0; i < HASH_8BITS; i++) {
		init_list_head(&mac_port_map.hash_table[i]);
	}

	pthread_mutex_init(&mac_port_map.lock, NULL);

	pthread_create(&mac_port_map.thread, NULL, sweeping_mac_port_thread, NULL);
}

// destroy mac_port table
void destory_mac_port_table()
{
	pthread_mutex_lock(&mac_port_map.lock);
	mac_port_entry_t *entry, *q;
	for (int i = 0; i < HASH_8BITS; i++) {
		list_for_each_entry_safe(entry, q, &mac_port_map.hash_table[i], list) {
			list_delete_entry(&entry->list);
			free(entry);
		}
	}
	pthread_mutex_unlock(&mac_port_map.lock);
}

// lookup the mac address in mac_port table
// FDB中寻找目的地址+port是否存在？
iface_info_t *lookup_port(u8 mac[ETH_ALEN])
{
	// iface_info_t *iface = NULL;
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return iface;
	
	iface_info_t *iface = NULL;
	mac_port_entry_t *entry,*q;
	for (int i = 0; i < HASH_8BITS; i++) {
		list_for_each_entry_safe(entry, q, &mac_port_map.hash_table[i], list) {
			int cmp = memcmp((void*)entry->mac,(void*)mac,sizeof(u8)*ETH_ALEN);   //ETH_ALEN==6
			if(cmp==0) return entry->iface;
		}
	}
	return iface;
}

// insert the mac -> iface mapping into mac_port table
//FDB插入packet源地址+port
void insert_mac_port(u8 mac[ETH_ALEN], iface_info_t *iface)
{
	// fprintf(stdout, "TODO: implement this function please.\n");'
	mac_port_entry_t *entry = malloc(sizeof(mac_port_entry_t));
	bzero(entry, sizeof(mac_port_entry_t));
	time_t now = time(NULL);
	entry->visited = now;
	memcpy(entry->mac,mac,sizeof(u8)*ETH_ALEN);
	// memcpy(entry->iface,iface,sizeof(iface_info_t));
	entry->iface=iface;
	list_add_tail(&entry->list,&mac_port_map.hash_table[0]);           //我加在第一张hash table可以？   为什么hash_table存储了所有的entry？
	
}

// dumping mac_port table
//stdout输出 mac-port fdb表
void dump_mac_port_table()
{
	mac_port_entry_t *entry = NULL;
	time_t now = time(NULL);

	fprintf(stdout, "dumping the mac_port table:\n");
	pthread_mutex_lock(&mac_port_map.lock);
	for (int i = 0; i < HASH_8BITS; i++) {
		list_for_each_entry(entry, &mac_port_map.hash_table[i], list) {
			fprintf(stdout, ETHER_STRING " -> %s, %d\n", ETHER_FMT(entry->mac), \
					entry->iface->name, (int)(now - entry->visited));
		}
	}

	pthread_mutex_unlock(&mac_port_map.lock);
}

// sweeping mac_port table, remove the entry which has not been visited in the
// last 30 seconds.
int sweep_aged_mac_port_entry()
{
	// int n = 0;
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return n;
	int n=0;
	mac_port_entry_t *entry, *q;
	time_t now = time(NULL);
	for (int i = 0; i < HASH_8BITS; i++) {
		list_for_each_entry_safe(entry,q, &mac_port_map.hash_table[i],list) {
			if((int)(now - entry->visited) >= MAC_PORT_TIMEOUT){
				n = entry->iface->index;
				list_delete_entry(&entry->list);
				free(entry);
				return n;
			}
		}
	}
	return n;
}

// sweeping mac_port table periodically, by calling sweep_aged_mac_port_entry
//此线程函数 1s执行一次sweep_aged_mac_port_entry
void *sweeping_mac_port_thread(void *nil)
{
	while (1) {
		sleep(1);
		int n = sweep_aged_mac_port_entry();

		if (n > 0)
			log(DEBUG, "%d aged entries in mac_port table are removed.", n);
	}
	return NULL;
}
