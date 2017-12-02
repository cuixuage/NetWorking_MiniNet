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
// mapping_entry 所有结构体IP + PORT全部按照网络字节序存储 这样只需要在printf时 进行转换即可
// 注:网络packet中的整型变量保存早本机应该是需要全部转化为主机字节序的
// #include <sys/socket.h>
// #include <netinet/in.h>   //为了使用inet_addr(char *)函数 以网络字节序保存long  
// #include <arpa/inet.h>
   
static struct nat_table nat;

// hash address and port            //这个hash如何使用？
int nat_hash(u32 addr, u16 port)
{
	int val = hash8((char *)&addr, 4) ^ hash8((char *)&port, 2);

	return val;
}

//尚未完成
// check whether the flow is finished according to FIN bit and sequence number
int is_flow_finished(struct nat_connection *conn)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return 0;
	if(conn->internal_fin==1 || conn->external_fin==1) return 0;   //stop
	else return 1;
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

//DIR_IN  通过NAT assigned port寻找映射nat-mapping
// lookup the corresponding map from mapping_list according to external_port
struct nat_mapping *nat_lookup_external(struct list_head *mapping_list, u16 external_port)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return NULL;
	struct nat_mapping *mapping_entry, *q;
	list_for_each_entry_safe(mapping_entry, q, mapping_list, list) {
		if(mapping_entry->external_port == external_port)
			return mapping_entry;
	}
	return NULL;
}

//DIR_OUT 通过internal——ip+port寻找映射nat-mapping
// lookup the corresponding map from mapping_list according to internal_ip and
// internal_port
struct nat_mapping *nat_lookup_internal(struct list_head *mapping_list,u32 internal_ip, u16 internal_port)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return NULL;
	struct nat_mapping *mapping_entry, *q;
	list_for_each_entry_safe(mapping_entry, q, mapping_list, list) {
		if(mapping_entry->internal_ip == internal_ip &&
							mapping_entry->internal_port == internal_port)
			return mapping_entry;
	}
	return NULL;
}

// select an external port from the port pool
u16 assign_external_port()
{
	// u16 port = 0;
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return port;
	int i;
	u8 *portspool = nat.assigned_ports;					
	for(i=NAT_PORT_MIN; i<NAT_PORT_MAX ;++i)   // [] 优先级大于 成员运算符.  先取得assigned_ports的首部地址
		if( *(portspool+i) == 0){
			*(portspool+i)=1;
			break;
		}
	if(i!=NAT_PORT_MAX) return (u16)(i);
	else  return (u16)-1;
}

// free the port
void free_port(u16 port)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	u8 *portspool = nat.assigned_ports;	
	if(port >=NAT_PORT_MIN && port <= NAT_PORT_MAX)
		portspool[port]=0;
}
  
//nat_mapping 加入新的映射关系 DIR_IN   (from the SYN packet of the connection??   如何理解？是第一次握手数据包吗？)
// insert the new connection into mapping_list
// the internal_ip & internal_port are from the SYN packet of the connection
// the external_ip is the ip address of the external interface, the
// external_port is assigned by nat
struct nat_mapping *nat_insert_mapping(struct list_head *mapping_list, u32 internal_ip, u16 internal_port)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return NULL;
	
	struct nat_mapping *mapping_entry;
	mapping_entry = malloc(sizeof(struct nat_mapping));
	bzero(mapping_entry, sizeof(struct nat_mapping));
	
	mapping_entry->internal_ip = internal_ip;
	mapping_entry->internal_port = internal_port;
	mapping_entry->external_ip = inet_addr(nat.external_iface->ip_str);       // . -> 是同级优先级 从左向右结合
	mapping_entry->external_port = htons(assign_external_port()); 					//打算在nat表中全部按照网络字节序存储
	printf("assigned port:%u\n",ntohs(mapping_entry->external_port));				
	
	time_t now = time(NULL);
	mapping_entry->update_time = now;
	
	list_add_tail(&mapping_entry->list,mapping_list);                     //&优先级低于->	
	return mapping_entry;
}

// determine the direction of the packet, DIR_IN / DIR_OUT / DIR_INVALID
static int get_packet_direction(char *packet)
{
	// fprintf(stdout, "todo: implement this function please.\n");
	// return dir_invalid;

	pthread_mutex_lock(&nat.lock);
	struct iphdr *ip = packet_to_ip_hdr(packet);
	u32 daddr = ntohl(ip->daddr); 
	u32 external_ip = ntohl(inet_addr(nat.external_iface->ip_str));     //NAT 公网物理接口->IP
	pthread_mutex_unlock(&nat.lock);
	if(daddr == external_ip) return DIR_IN;          					//如何判断packet 为DIR_INVAILD  遍历源地址是否在内部网络？
	else return DIR_OUT;

}
 
//更新 nat mapping  SYN Seq ACK  
// update statistics of the tcp connection    //nat_mapping connection 更新？？？
void nat_update_tcp_connection(char *packet, struct nat_mapping *mapping, int dir)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	struct nat_connection* connection = &(mapping->conn);
	struct tcphdr* tcp_hdr = packet_to_tcp_hdr(packet);
	if(dir == DIR_IN){
		connection->external_seq_end = tcp_hdr->seq;
		connection->external_ack = tcp_hdr->seq;
	}
	else if(dir ==DIR_OUT){
		connection->internal_seq_end = tcp_hdr->seq;
		connection->internal_ack = tcp_hdr->seq;
	}
}

//len iface参数怎么使用？
// find the mapping corresponding to the packet from nat table 
struct nat_mapping *nat_get_mapping_from_packet(char *packet, int len, iface_info_t *iface, int dir)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return NULL;
	struct nat_mapping* mapping_entry = malloc(sizeof(struct nat_mapping));
	bzero(mapping_entry, sizeof(struct nat_mapping));
	struct tcphdr* tcp_hdr = packet_to_tcp_hdr(packet);
	u16 des_port = tcp_hdr->dport;										//network to host (short用于port)
	u16 src_port = tcp_hdr->sport;
	struct iphdr *ip = packet_to_ip_hdr(packet);
	u32 internal_ip = ip->saddr; 
	if(dir==DIR_IN){   													//通过目的port寻找
		for (int i = 0; i < HASH_8BITS; i++) {
			struct list_head *mapping_list = &nat.nat_mapping_list[i];
			mapping_entry = nat_lookup_external(mapping_list,des_port);
			if(mapping_entry != NULL) 
				return mapping_entry;
		}
	}	
	else{ 																//DIR_OUT 通过internal_IP +internal_port寻找
		for (int i = 0; i < HASH_8BITS; i++) {
			struct list_head *mapping_list = &nat.nat_mapping_list[i];
			mapping_entry = nat_lookup_internal(mapping_list,internal_ip,src_port);
			if(mapping_entry != NULL) 
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
	
	struct in_addr myaddr;
	myaddr.s_addr = ip_hdr->saddr;
	char* myip = inet_ntoa(myaddr);											//网络字节序的long 转化为点分十进制
	
	u32 NAT_ip = inet_addr(nat.external_iface->ip_str);   			//inet_addr 直接将点分十进制IP转化为网络字节序long
	// printf("hostNAT_IP:%x\n",NAT_ip);
	u32 srcip = ip_hdr->saddr;
	u16 srcport = tcp_hdr->sport;
	
	struct nat_mapping* mapping_entry = nat_get_mapping_from_packet(packet,len,iface,dir);
		
	if(dir == DIR_IN && mapping_entry != NULL){												//更换目的地址+目的port
		u32 des_ip = mapping_entry->internal_ip;						
		u16 des_port = mapping_entry->internal_port;
		ip_hdr->daddr = des_ip;
		tcp_hdr->dport = des_port;
#if 1		
		//测试 update packet IP+PORT
		myaddr.s_addr = ip_hdr->daddr ;
		char* myip = inet_ntoa(myaddr);
		printf("DIR_IN update packet ok,desIP=%s,desPORT=%u\n",myip,tcp_hdr->dport);
#endif		
	}
	else{								//更换源地址+源port    //加入新的映射关系 nat_insert_mapping
										//这里我向table[0]中插入了
		if(mapping_entry == NULL) {
			mapping_entry = nat_insert_mapping(&nat.nat_mapping_list[0],srcip,srcport);
			printf("insert mapping ok,srcip=%s,sport=%u,assigned=%u\n",myip,ntohs(srcport),ntohs(mapping_entry->external_port));
		}
		else{
#if 0
			//测试 nat_get_mapping_from_packet  mapping_entry 
			myaddr.s_addr = htonl(mapping_entry->internal_ip);
			char* myip = inet_ntoa(myaddr);	
			printf("DIR_OUT get mapping ok,srcip=%s,sport=%u,assignPORT=%u\n",myip,mapping_entry->internal_port,mapping_entry->external_port);
#endif
			ip_hdr->saddr = NAT_ip;
			tcp_hdr->sport = mapping_entry->external_port;
#if 1			
			//测试 update packet IP+PORT
			myaddr.s_addr =ip_hdr->saddr;
			char* myip = inet_ntoa(myaddr);
			printf("DIR_OUT update packet ok,assignIP=%s,assignPORT=%u\n",myip,ntohs(tcp_hdr->sport));
#endif
			}
	}
	//更改IP TCP协议的校验和  头部内容的变换影响校验和的计算？
	ip_hdr->checksum = ip_checksum(ip_hdr);
	tcp_hdr->checksum = tcp_checksum(ip_hdr,tcp_hdr);
	ip_send_packet(packet,len);		
	
#if 0	
	//iface_send_packet 是直接向物理接口发送数据 packet是完整的 例如:交换机的broadcast ？？？
	if(dir==DIR_IN){								
		iface_send_packet(nat.internal_iface,packet,len);
		printf("send IN_packet ok\n");
	}
	else{
		iface_send_packet(nat.external_iface,packet,len);
		printf("send OUT_packet ok\n");
	}
#endif
}

void nat_translate_packet(iface_info_t *iface, char *packet, int len)
{
	int dir = get_packet_direction(packet);
	printf("get dir=%d\n",dir);
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

	do_translation(iface, packet, len, dir);				//保证packet中整数都是网络字节序		
	
}

//指针函数 返回指针
//对于超时的连接 删除映射关系
// nat timeout thread: find the finished flows, remove them and free port
// resource
void *nat_timeout()
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	// return NULL;
	time_t now = time(NULL);
	for (int i = 0; i < HASH_8BITS; i++) {
		struct list_head *head = &nat.nat_mapping_list[i];
		struct nat_mapping *mapping_entry, *q;
		list_for_each_entry_safe(mapping_entry, q, head, list) {
			if(mapping_entry->update_time - now >= 60){
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

	nat.internal_iface = if_name_to_iface("n1-eth0");             //内部网络物理接口 ？ 如何理解？  路由器+交换机？
	nat.external_iface = if_name_to_iface("n1-eth1");			  //公网物理接口 Ip
	printf("external_ip:%s\n",nat.external_iface->ip_str);
	
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
		list_for_each_entry_safe(mapping_entry, q, head, list) {    //list_for_each_entry_safe 被define为for循环
			list_delete_entry(&mapping_entry->list);
			free(mapping_entry);
		}
	}

	pthread_kill(nat.thread, SIGTERM);

	pthread_mutex_unlock(&nat.lock);
}
