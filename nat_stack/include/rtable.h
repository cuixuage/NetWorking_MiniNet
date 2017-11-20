#ifndef __RTABLE_H__
#define __RTABLE_H__

#include "base.h"
#include "types.h"

#include "list.h"

// structure of ip forwarding table
// note: 1, the table supports only ipv4 address;
// 		 2, addresses are stored in host byte order.
typedef struct {
	struct list_head list;
	u32 dest;				// destination ip address (could be network or host)
	u32 gw;					// ip address of next hop (will be 0 if dest is in 
							// the same network with iface)
	u32 mask;				// network mask of dest
	int flags;				// flags (could be omitted here)
	char if_name[16];		// name of the interface
	iface_info_t *iface;	// pointer to the interface structure
} rt_entry_t;

extern struct list_head rtable;

void init_rtable();
rt_entry_t *longest_prefix_match(u32 ip);

#endif
