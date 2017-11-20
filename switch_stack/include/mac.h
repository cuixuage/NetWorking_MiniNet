#ifndef __MAC_H__
#define __MAC_H__

#include "base.h"
#include "hash.h"
#include "list.h"

#include <pthread.h>
#include <unistd.h>

#define MAC_PORT_TIMEOUT 30

// mac->port(iface) mapping entry
typedef struct {
	struct list_head list;	// list node used to link all entries
	uint8_t mac[ETH_ALEN];	// mac address
	iface_info_t *iface;	// pointer to iface
	time_t visited;			// when this entry is visited last time
} mac_port_entry_t;

typedef struct {
	struct list_head hash_table[HASH_8BITS];	// hash table to store all entries    //数组 256个list_head类型元素  存储all entry?
	pthread_mutex_t lock;		// mutex lock
	pthread_t thread;			// id of sweeping aged entry thread 
} mac_port_map_t;

void *sweeping_mac_port_thread(void *);
void init_mac_port_table();
void destory_mac_port_table();
void dump_mac_port_table();
iface_info_t *lookup_port(uint8_t mac[ETH_ALEN]);
void insert_mac_port(uint8_t mac[ETH_ALEN], iface_info_t *iface);
int sweep_aged_mac_port_entry();

#endif
