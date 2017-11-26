**  简单的socket通信 **
**分布式字符统计** 

需求:统计某个文件中的所有字母出现次数，并将工作合理分配到 conf 中出现的若干个IP 代表的 worker 中，并获取结果展示   
测试环境： mininet 搭建 1 个 switch+3 个 host(1 个 master+2 个 worker)   

把 master 作为 server 端，worker 作为 client 进行的实验  
-1.Server 多线程处理来自 client 的链接请求 避免的 accept 阻塞问题   
-2.server 发送 filename + file_start + file_end 到 client. Client 从给定的文件起始位置结束位置加以处理，并将 hash 结果返回到 server   
-3.server 端 pthread_join 等待最后一个的线程执行完毕后，再将 static ans 展示  