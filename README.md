# NetWorking_MiniNet  
UCAS Experiment  

**1.Switch_stack交换机实验  completed in 2017/11/8**    

实现对数据结构mac_port_map的所有操作，以及数据包的转发和广播操作  
思路:  
1.对于收到的packet，根据其des_mac在FDB表中进行查询，存在对应的interface则转发，否则在局域网内广播此packet  
2.对于收到的packet，如果其src_mac不在FDB表中则认为是新加入的主机，将其src_mac与interface物理端口的映射关系加入FDB     
3.key points:  当交换机从某一个interface收到来自src_mac地址的A主机的packet时，可以确定: des_mac为A主机的packet从其对应的inerface转出即可到达A主机   
实现函数: 
-iface_info_t *lookup_port(u8 mac[ETH_ALEN]);  
-void insert_mac_port(u8 mac[ETH_ALEN], iface_info_t *iface);  
-int sweep_aged_mac_port_entry();  
-void broadcast_packet(iface_info_t *iface, const char *packet, int len);   
-void handle_packet(iface_info_t *iface, char *packet, int len);    
  
实验iperf结果:   
switch:  
[ ID] Interval       Transfer     Bandwidth  
[ 13]  0.0-30.2 sec  33.5 MBytes  9.29 Mbits/sec  
boardcast:    
[ ID] Interval       Transfer     Bandwidth   
[ 13]  0.0-30.3 sec  20.9 MBytes  5.78 Mbits/se  

**3.NAT_stack NAT地址转换实验  completed in 2017/11/20**  

实现对于内网外网IP_PORT hash表NAT_mapping_entry操作,从而实现数据包转发(更换IP+PORT,重新计算checksum)    
思路:    
1.packet方向为DIR_IN     
根据packet的tcp协议中des_port，从映射表中查找映射，得到局域网内部主机的IP+PORT，替换des_IP, des_port,重新计算IP\TCP协议的校验和，转发packet    
2.packet方向是DIR_OUT     
根据packet中的src_IP,src_PORT，从映射表中查找映射，如果不存在则加入映射表，得到NAT新分配的assgined_port。替换src_IP为NAT公网IP，src_port为assgined_port,重新计算校验和，转发packet     
3.根据packet的des_IP == NAT公网IP判断packet发送方向       
4.超时连接的timeout操作，free删除链接    
实现函数:   
-struct nat_mapping *nat_lookup_external(struct list_head *mapping_list, u16 external_port);  
-struct nat_mapping *nat_lookup_internal(struct list_head *mapping_list,u32 internal_ip, u16 internal_port);  
-u16 assign_external_port();  
-void free_port(u16 port);  
-struct nat_mapping *nat_insert_mapping(struct list_head *mapping_list, u32 internal_ip, u16 internal_port);  
-static int get_packet_direction(char *packet);  
-void nat_update_tcp_connection(char *packet, struct nat_mapping *mapping, int dir);  
-struct nat_mapping *nat_get_mapping_from_packet(char *packet, int len, iface_info_t *iface, int dir);  
-void do_translation(iface_info_t *iface, char *packet, int len, int dir);  
-void *nat_timeout();  

实验结果:  
h2启动simpleHTTPServer ;h1 wget h2_IP   
获取到当前目录所有文件并写入到index.html  

**5.Socket_example 分布式字符统计实验  completed in 2017/11/26**  

需求:统计某个文件中的所有字母出现次数，并将工作合理分配到 conf 中出现的若干个IP 代表的 worker 中，并获取结果展示   
测试环境： mininet 搭建 1 个 switch+3 个 host(1 个 master+2 个 worker)   

把 master 作为 server 端，worker 作为 client 进行的实验  
-1.Server 多线程处理来自 client 的链接请求 避免的 accept 阻塞问题   
-2.server 发送 filename + file_start + file_end 到 client. Client 从给定的文件起始位置结束位置加以处理，并将 hash 结果返回到 server   
-3.server 端 pthread_join 等待最后一个的线程执行完毕后，再将 static ans 展示 
