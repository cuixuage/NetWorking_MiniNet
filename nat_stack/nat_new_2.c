/*

TIME : 2017/12/3  19:24
This version use hash!

*/
#include "nat.h"
#include "ip.h"
#include "icmp.h"
#include "tcp.h"
#include "rtable.h"
#include "log.h"
#include "packet.h"    //iface_send_packet()
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static struct nat_table nat;

// hash address and port            
int nat_hash(u32 addr, u16 port)
{
	int val = hash8((char *)&addr, 4) ^ hash8((char *)&port, 2);

	return val;
}

//need to modify
// check whether the flow is finished according to FIN bit and sequence number
int is_flow_finished(struct nat_connection *conn)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return 0;
	//if(conn->internal_fin==1 || conn->external_fin==1) return 0;   //stop
	if(conn -> external_fin == 0x01 && conn-> internal_fin == 0x01 && conn->external_ack == conn->internal_seq_end+1 && conn->internal_ack == conn->external_seq_end+1)
		return 1;
	else return 0;
}

// get the interface from iface name
static iface_info_t *if_name_to_iface(const char *if_name)
{
	iface_info_t *iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		if (strcmp(iface->name, if_name) == 0)
			return iface;
	}

	log(ERROR, "Could not find the desired interface according to if_name '%s'", if_name);
	return NULL;
}

// lookup the corresponding map from mapping_list according to external_port
struct nat_mapping *nat_lookup_external(struct list_head *mapping_list, u16 external_port)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return NULL;
	u32 external_ip=nat.external_iface->ip;
	int val=nat_hash(external_ip, external_port)%HASH_8BITS;
	struct nat_mapping *mapping_entry, *q;
	struct list_head *head = &mapping_list[val];
	list_for_each_entry_safe(mapping_entry, q, head, list) {
		if(mapping_entry->external_port == external_port)
			return mapping_entry;
	}
	return NULL;
}

// lookup the corresponding map from mapping_list according to internal_ip and
// internal_port
struct nat_mapping *nat_lookup_internal(struct list_head *mapping_list,u32 internal_ip, u16 internal_port)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return NULL;
	struct nat_mapping *mapping_entry, *q;
	int val=nat_hash(internal_ip, internal_port)%HASH_8BITS;
	struct list_head *head = &mapping_list[val];
	list_for_each_entry_safe(mapping_entry, q, head, list) {
		if(mapping_entry->internal_ip == internal_ip &&
							mapping_entry->internal_port == internal_port)
			return mapping_entry;
	}
	return NULL;
}

// select an external port from the port pool
u16 assign_external_port()
{
	u16 port = 0;
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return port;
	u16 i;					
	for(i=NAT_PORT_MIN; i<=NAT_PORT_MAX ;i++)   
	{	if( nat.assigned_ports[i] == 0){
			port = i;
			nat.assigned_ports[i] = 1;
			break;
		}
	}
	if(port == 0)
	{
		fprintf(stdout,"No Suitable Ports Can Be Used!\n");		
	}
	return port;
}

// free the port
void free_port(u16 port)
{
	// fprintf(stdout, "TODO: implement this function please.\n");	
	if(port >=NAT_PORT_MIN && port <= NAT_PORT_MAX)
		nat.assigned_ports[port]=0;
}
  

// insert the new connection into mapping_list
// the internal_ip & internal_port are from the SYN packet of the connection
// the external_ip is the ip address of the external interface, the
// external_port is assigned by nat
struct nat_mapping *nat_insert_mapping(struct list_head *mapping_list, u32 internal_ip, u16 internal_port)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return NULL;
	//ĞÂ½¨Ò»¸önat-mapping
	struct nat_mapping *mapping_entry;
	mapping_entry = malloc(sizeof(struct nat_mapping));
	bzero(mapping_entry, sizeof(struct nat_mapping));
	init_list_head(&(mapping_entry->list));
	//ÎªĞÂ½¨µÄ¹ØÏµ¸³Öµ
	mapping_entry->internal_ip = internal_ip;
	mapping_entry->internal_port = internal_port;
	//mapping_entry->external_ip = inet_addr(nat.external_iface->ip_str); 
	mapping_entry->external_ip = nat.external_iface->ip; 
	mapping_entry->external_port = htons(assign_external_port()); 								
	//½«ĞÂ½¨µÄ¹ØÏµ¼ÓÈëÓ³Éä±í
    int new_val=nat_hash(internal_ip,internal_port)%HASH_8BITS;
	list_add_tail(&(mapping_entry->list),&(mapping_list[new_val]));  


    //ĞÂ½¨Ò»¸önat_mapping
	struct nat_mapping * new_nat_mapping=malloc(sizeof(struct nat_mapping));
	bzero(new_nat_mapping, sizeof(struct nat_mapping));
	init_list_head(&(new_nat_mapping->list));
    //ÎªĞÂ½¨µÄ¹ØÏµÓ³Éä¸³Öµ
	new_nat_mapping->internal_ip=mapping_entry->internal_ip;
	new_nat_mapping->external_ip=mapping_entry->external_ip;
	new_nat_mapping->internal_port=mapping_entry->internal_port;
	new_nat_mapping->external_port=mapping_entry->external_port;
	//½«ĞÂ½¨µÄ¹ØÏµ¼ÓÈëÓ³Éä±í
	int val=nat_hash(new_nat_mapping->external_ip,new_nat_mapping->external_port)%HASH_8BITS;
	list_add_tail(&(new_nat_mapping->list), &(mapping_list[val]));

    //ÕâÀïµÄ¸üĞÂĞèÒªÃ´£¿£¿£¿
	time_t now = time(NULL);
	mapping_entry->update_time = now;                   	
	return mapping_entry;
}

// determine the direction of the packet, DIR_IN / DIR_OUT / DIR_INVALID
static int get_packet_direction(char *packet)
{
	// fprintf(stdout, "todo: implement this function please.\n");
	// return dir_invalid;
	//½âÎöÊı¾İ°üµÃµ½IPÍ·²¿£¬ÍøÂç×Ö½ÚĞò×ªÎªÖ÷»ú×Ö½ÚĞò£¬µÃµ½Ä¿µÄµØÖ·IP
	pthread_mutex_lock(&nat.lock);
	struct iphdr *ip = packet_to_ip_hdr(packet);
	u32 daddr = ntohl(ip->daddr); 
	//u32 external_ip = ntohl(inet_addr(nat.external_iface->ip_str));  
	u32 external_ip = nat.external_iface->ip;
	pthread_mutex_unlock(&nat.lock);
	if(daddr == external_ip) return DIR_IN;          					//IR_INVAILD
	else return DIR_OUT;

}
 
//update nat mapping  SYN Seq ACK  
// update statistics of the tcp connection   
void nat_update_tcp_connection(char *packet, struct nat_mapping *mapping, int dir)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	struct nat_connection* connection = &(mapping->conn);
	struct tcphdr* tcp_hdr = packet_to_tcp_hdr(packet);
	if(dir == DIR_IN){
		//connection->external_seq_end = tcp_hdr->seq;
		connection->external_fin = tcp_hdr->flags&TCP_FIN;
		//connection->external_seq_end = ntohs(tcp_hdr->seq);
		//connection->external_ack = ntohs(tcp_hdr->ack);
		connection->external_seq_end = tcp_hdr->seq;
		connection->external_ack = tcp_hdr->ack;
	
	}
	else if(dir ==DIR_OUT){
		//connection->internal_seq_end = tcp_hdr->seq;
		//connection->internal_ack = tcp_hdr->seq;
		connection->internal_fin = tcp_hdr->flags&TCP_FIN;
		connection->internal_seq_end = tcp_hdr->seq;
		connection->internal_ack = tcp_hdr->ack;
		//connection->internal_seq_end = ntohs(tcp_hdr->seq);
		//connection->internal_ack = ntohs(tcp_hdr->ack);
	}
}

//how to use len and ifaceŸ
// find the mapping corresponding to the packet from nat table 
struct nat_mapping *nat_get_mapping_from_packet(char *packet, int len, iface_info_t *iface, int dir)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return NULL;
	//ĞÂ½¨Ò»¸önat-mapping
	//struct nat_mapping* mapping_entry = malloc(sizeof(struct nat_mapping));
	//bzero(mapping_entry, sizeof(struct nat_mapping));
    struct nat_mapping* mapping_entry = NULL;
	struct tcphdr* tcp_hdr = packet_to_tcp_hdr(packet);
	u16 des_port = tcp_hdr->dport;										
	u16 src_port = tcp_hdr->sport;
	struct iphdr *ip = packet_to_ip_hdr(packet);
	u32 internal_ip = ip->saddr; 
	if(dir==DIR_IN){   													
	//	for (int i = 0; i < HASH_8BITS; i++) {
	//		struct list_head *mapping_list = &nat.nat_mapping_list[i];
	//		mapping_entry = nat_lookup_external(mapping_list,des_port);
            mapping_entry = nat_lookup_external(nat.nat_mapping_list,des_port);
	    	if(mapping_entry != NULL) 
				return mapping_entry;
		//}
	}	
	else{ 																
		//for (int i = 0; i < HASH_8BITS; i++) {
		//	struct list_head *mapping_list = &nat.nat_mapping_list[i];
		    mapping_entry = nat_lookup_internal(nat.nat_mapping_list, internal_ip, src_port);
			if(mapping_entry != NULL) 
				return mapping_entry;
			else {
				mapping_entry = nat_insert_mapping(nat.nat_mapping_list, internal_ip, src_port);
				return mapping_entry;
			}
	}
	return NULL;
}

// do translation for the packet: replace the ip/port, recalculate ip & tcp
// checksum, update the statistics of the tcp connection
void do_translation(iface_info_t *iface, char *packet, int len, int dir)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
	struct tcphdr* tcp_hdr = packet_to_tcp_hdr(packet);	
	u32 NAT_ip = inet_addr(nat.external_iface->ip_str);   			
	u32 srcip = ip_hdr->saddr;
	u16 srcport = tcp_hdr->sport;
	
	struct nat_mapping* mapping_entry = nat_get_mapping_from_packet(packet,len,iface,dir);
		
	if(dir == DIR_IN && mapping_entry != NULL){												
		//u32 des_ip = mapping_entry->internal_ip;						
		//u16 des_port = mapping_entry->internal_port;
		//ip_hdr->daddr = des_ip;   //transfer;no h1 addr,is eth0 addr
		//tcp_hdr->dport = des_port; 
		ip_hdr->daddr = mapping_entry->internal_ip;
		tcp_hdr->dport = mapping_entry->internal_port;
	}
	else{								   
									
		//if(mapping_entry == NULL) {
			//mapping_entry = nat_insert_mapping(&nat.nat_mapping_list[0],srcip,srcport);///have question!!!!//////need to modify!!!
		//	mapping_entry = nat_insert_mapping(nat.nat_mapping_list,srcip,srcport);
			//printf("insert mapping ok,srcip=%s,sport=%u,assigned=%u\n",myip,ntohs(srcport),ntohs(mapping_entry->external_port));
		//}
		//else{
			ip_hdr->saddr = NAT_ip;
			tcp_hdr->sport = mapping_entry->external_port;
		//}
	}
	//ÖØĞÂ¼ÆËãIP/TCPĞ£ÑéºÍ
	ip_hdr->checksum = ip_checksum(ip_hdr);
	tcp_hdr->checksum = tcp_checksum(ip_hdr,tcp_hdr);	
	//¸üĞÂ½ÓÊÕµ½Êı¾İ°üµÄÊ±¼ä
	time(&mapping_entry->update_time);
    //·¢ËÍÊı¾İ°ü
	ip_send_packet(packet,len);	
	//¸üĞÂtcpÁ¬½Ó
   // nat_update_tcp_connection(packet,mapping_entry,dir);
}

void nat_translate_packet(iface_info_t *iface, char *packet, int len)
{
	int dir = get_packet_direction(packet);
//	printf("get dir=%d\n",dir);
	if (dir == DIR_INVALID) {
		log(ERROR, "invalid packet direction, drop it.");
		icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
		free(packet);
		return ;
	}

	struct iphdr *ip = packet_to_ip_hdr(packet);
	if (ip->protocol != IPPROTO_TCP) {
		log(ERROR, "received non-TCP packet (0x%0hhx), drop it", ip->protocol);
		free(packet);
		return ;
	}

	do_translation(iface, packet, len, dir);						
	
}

// nat timeout thread: find the finished flows, remove them and free port
// resource
void *nat_timeout()
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return NULL;
	//Ë«·½¶¼ÒÑ¾­·¢ËÍFINÇÒ»Ø¸´ÏàÓ¦ACKµÄÁ¬½Ó£¬Ò»·½·¢ËÍRST°üµÄÁ¬½Ó£¬ÈÏÎª½áÊø
	//Ë«·½ÒÑ¾­³¬¹ı60ÃëÎ´´«ÊäÊı¾İµÄÁ¬½Ó£¬ÈÏÎª½áÊø
	//¶ÔÓÚÒÑ¾­½áÊøµÄÁ¬½Ó£¬¿ÉÒÔÊÕ»ØÒÑ·ÖÅäµÄportºÅ£¬ÊÍ·ÅÁ¬½ÓÓ³Éä×ÊÔ´
	time_t now = time(NULL);
	for (int i = 0; i < HASH_8BITS; i++) {
		struct list_head *head = &nat.nat_mapping_list[i];
		struct nat_mapping *mapping_entry, *q;
		list_for_each_entry_safe(mapping_entry, q, head, list) {
			if(0!=is_flow_finished(&(mapping_entry->conn)) || mapping_entry->update_time - now >= 60){
				free_port(mapping_entry->external_port);
				list_delete_entry(&mapping_entry->list);
				free(mapping_entry);
			}
		}
	}
	return NULL;
}

// initialize nat table
void nat_table_init()
{
	memset(&nat, 0, sizeof(nat));

	for (int i = 0; i < HASH_8BITS; i++)
		init_list_head(&nat.nat_mapping_list[i]);

	nat.internal_iface = if_name_to_iface("n1-eth0");            
	nat.external_iface = if_name_to_iface("n1-eth1");			  
	//printf("external_ip:%s\n",nat.external_iface->ip_str);
	
	if (!nat.internal_iface || !nat.external_iface) {
		log(ERROR, "Could not find the desired interfaces for nat.");
		exit(1);
	}

	memset(nat.assigned_ports, 0, sizeof(nat.assigned_ports));

	pthread_mutex_init(&nat.lock, NULL);

	pthread_create(&nat.thread, NULL, nat_timeout, NULL);
	printf("init table ok\n");
}

// destroy nat table
void nat_table_destroy()
{
	pthread_mutex_lock(&nat.lock);

	for (int i = 0; i < HASH_8BITS; i++) {
		struct list_head *head = &nat.nat_mapping_list[i];
		struct nat_mapping *mapping_entry, *q;
		list_for_each_entry_safe(mapping_entry, q, head, list) {    
			list_delete_entry(&mapping_entry->list);
			free(mapping_entry);
		}
	}

	pthread_kill(nat.thread, SIGTERM);

	pthread_mutex_unlock(&nat.lock);
}