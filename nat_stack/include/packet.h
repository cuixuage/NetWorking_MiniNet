#ifndef __PACKET_H__
#define __PACKET_H__

#include "base.h"
#include "types.h"

// iface send an packet, given all the fields of the packet are ready
void iface_send_packet(iface_info_t *iface, char *packet, int len);

#endif
