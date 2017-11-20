#ifndef __TCP_H__
#define __TCP_H__

#include "types.h"
#include "ip.h"
#include "checksum.h"

#include <endian.h>

// format of standard tcp header 
struct tcphdr {
	u16 sport;		// source port 
	u16 dport;		// destination port
	u32 seq;			// sequence number
	u32 ack;			// acknowledgement number
# if __BYTE_ORDER == __LITTLE_ENDIAN
	u8 x2:4;			// (unused)
	u8 off:4;			// data offset
# elif __BYTE_ORDER == __BIG_ENDIAN
	u8 off:4;			// data offset
	u8 x2:4;			// (unused)
# endif
	u8 flags;                     //标志位flags   例如: 如果是0x01 那么就是TCP_FIN？？
# define TCP_FIN	0x01
# define TCP_SYN	0x02
# define TCP_RST	0x04
# define TCP_PSH	0x08
# define TCP_ACK	0x10
# define TCP_URG	0x20
	u16 rwnd;			// receiving window
	u16 checksum;		// checksum
	u16 urp;			// urgent pointer
} __attribute__((packed));                    //__attribute__作用是什么？？？

#define TCP_HDR_OFFSET 5
#define TCP_BASE_HDR_SIZE 20
#define TCP_HDR_SIZE(tcp) (tcp->off * 4)

#define TCP_DEFAULT_WINDOW 65535

//从传输层来看，是发送方主机中的一个进程与接收方主机中的一个进程在交换数据
static inline struct tcphdr *packet_to_tcp_hdr(char *packet)         //传输层协议 有port记录  网络层协议IP没有PORT记录？？  tcp ip协议详解得得看看  这是怎么转换的？？ 额外加入了空间？
{
	struct iphdr *ip = packet_to_ip_hdr(packet);
	return (struct tcphdr *)((char *)ip + IP_HDR_SIZE(ip));
}

static inline u16 tcp_checksum(struct iphdr *ip, struct tcphdr *tcp)
{
	u16 tmp = tcp->checksum;
	tcp->checksum = 0;

	u16 reserv_proto = ip->protocol;
	u16 tcp_len = ntohs(ip->tot_len) - IP_HDR_SIZE(ip);

	u32 sum = ip->saddr + ip->daddr + htons(reserv_proto) + htons(tcp_len);
	u16 cksum = checksum((u16 *)tcp, (int)tcp_len, sum);

	tcp->checksum = tmp;

	return cksum;
}

void tcp_copy_flags_to_str(u8 flags, char buf[]);

#endif

/*
标志位:
SYN表示建立连接，

FIN表示关闭连接，

ACK表示响应，

PSH表示有数据传输   即packet大小不为零

RST表示连接重置



第一次握手：主机A发送位码为syn＝1，随机产生seq number=1234567的数据包到服务器，主机B由SYN=1知道，A要求建立联机；

第二次握手：主机B收到请求后要确认联机信息，向A发送ack number=(主机A的seq+1)，syn=1，ack=1，随机产生seq=7654321的包；

第三次握手：主机A收到后检查ack number是否正确，即第一次发送的seq number+1，以及位码ack是否为1，若正确，主机A会再发送ack number=(主机B的seq+1)，ack=1，主机B收到后确认seq值与ack=1则连接建立成功


*/
