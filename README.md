# NetWorking_MiniNet  
UCAS Experiment  

**1.Switch_stack  completed in 2017/11/8**    
实现对数据结构mac_port_map的所有操作，以及数据包的转发和广播操作  
•iface_info_t *lookup_port(u8 mac[ETH_ALEN]);  
•void insert_mac_port(u8 mac[ETH_ALEN], iface_info_t *iface);  
•int sweep_aged_mac_port_entry();  
•void broadcast_packet(iface_info_t *iface, const char *packet, int len);   
•void handle_packet(iface_info_t *iface, char *packet, int len);    
  
iperf结果:   
switch:  
[ ID] Interval       Transfer     Bandwidth  
[ 13]  0.0-30.2 sec  33.5 MBytes  9.29 Mbits/sec  
boardcast:    
[ ID] Interval       Transfer     Bandwidth   
[ 13]  0.0-30.3 sec  20.9 MBytes  5.78 Mbits/se  
