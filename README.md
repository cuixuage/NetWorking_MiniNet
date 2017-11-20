# NetWorking_MiniNet  
UCAS Experiment  

**1.Switch_stack交换机实验  completed in 2017/11/8**    

实现对数据结构mac_port_map的所有操作，以及数据包的转发和广播操作 
实现函数: 
+iface_info_t *lookup_port(u8 mac[ETH_ALEN]);  
+void insert_mac_port(u8 mac[ETH_ALEN], iface_info_t *iface);  
+int sweep_aged_mac_port_entry();  
+void broadcast_packet(iface_info_t *iface, const char *packet, int len);   
+void handle_packet(iface_info_t *iface, char *packet, int len);    
  
实验iperf结果:   
switch:  
[ ID] Interval       Transfer     Bandwidth  
[ 13]  0.0-30.2 sec  33.5 MBytes  9.29 Mbits/sec  
boardcast:    
[ ID] Interval       Transfer     Bandwidth   
[ 13]  0.0-30.3 sec  20.9 MBytes  5.78 Mbits/se  

**3.NAT_stack NAT地址转换实验  completed in 2017/11/20**  

实现对于内网外网IP_PORT hash表NAT_mapping_entry操作,从而实现数据包转发(更换IP+PORT,重新计算checksum)   
实现函数:  
+struct nat_mapping *nat_lookup_external(struct list_head *mapping_list, u16 external_port);  
+struct nat_mapping *nat_lookup_internal(struct list_head *mapping_list,u32 internal_ip, u16 internal_port);  
+u16 assign_external_port();  
+void free_port(u16 port);  
+struct nat_mapping *nat_insert_mapping(struct list_head *mapping_list, u32 internal_ip, u16 internal_port);  
+static int get_packet_direction(char *packet);  
+void nat_update_tcp_connection(char *packet, struct nat_mapping *mapping, int dir);  
+struct nat_mapping *nat_get_mapping_from_packet(char *packet, int len, iface_info_t *iface, int dir);  
+void do_translation(iface_info_t *iface, char *packet, int len, int dir);  
+void *nat_timeout();  

实验结果:  
h2启动simpleHTTPServer ;h1 wget h2_IP   
获取到当前目录所有文件并写入到index.html  
