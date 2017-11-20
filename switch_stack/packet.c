#include "packet.h"
#include "types.h"
#include "ether.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>

extern ustack_t *instance;     //main.c 用户栈保存物理实际端口？还是整个fdb表的所有端口

// send the packet out from iface
void iface_send_packet(iface_info_t *iface, char *packet, int len)
{
	struct sockaddr_ll addr;
	memset(&addr, 0, sizeof(struct sockaddr_ll));
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = iface->index;
	addr.sll_halen = ETH_ALEN;
	addr.sll_protocol = htons(ETH_P_ARP);
	struct ether_header *eh = (struct ether_header *)packet;
	memcpy(addr.sll_addr, eh->ether_dhost, ETH_ALEN);

	if (sendto(iface->fd, packet, len, 0, (struct sockaddr *)&addr,
				sizeof(struct sockaddr_ll)) < 0) {
 		perror("Send raw packet failed");
	}
}

// broadcast the packet among all the interfaces except the one receiving the
// packet, and free memory of the packet
//在所有的接口中广播(不包含当前接口)
void broadcast_packet(iface_info_t *iface, char *packet, int len)
{
	iface_info_t *tx_iface = NULL;
	list_for_each_entry(tx_iface, &instance->iface_list, list) {
		if (tx_iface->index == iface->index)
			continue;

		iface_send_packet(tx_iface, packet, len);
	}
}
