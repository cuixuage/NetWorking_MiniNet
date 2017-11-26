#include <stdio.h>
#include <string.h>
#include <sys/socket.h>			
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h> //头文件
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>


struct train{
	int _newfd;   		 //socket通信fd
	long _start;		 //task起始位置
	long _end;
};
static int ans[26]={0};				//保存所有client返回的结果
pthread_mutex_t output_lock;		//加锁 线程互斥 避免多个线程同时对于最终结果写操作

int countIP(){
	char buf[100]={0};
	int countIP=0;
	char* conf = "../workers.conf";
	int conf_fd = open(conf,O_RDONLY);
	read(conf_fd,buf,sizeof(buf));
	for(int i=0;i<strlen(buf);i++){
		if(buf[i]=='\n')
			countIP++;
	}
	printf("ReadIP_count = %d\n",countIP);
	return countIP;
}	
long startFile(int index,int allnum){          			//index序号由0开始的worker; allnum所有worker数量
	char* readfile="../war_and_peace.txt";
	struct stat statbuf;
	memset(&statbuf,0,sizeof(statbuf));
	stat(readfile,&statbuf);
	long len = statbuf.st_size;
	return (len/allnum)*index;
}
long endFile(int index,int allnum){          			
	char* readfile="../war_and_peace.txt";
	struct stat statbuf;
	memset(&statbuf,0,sizeof(statbuf));
	stat(readfile,&statbuf);
	long len = statbuf.st_size;
	return (len/allnum)*(index+1)-1;
}
long endFileFinal(){          			
	char* readfile="../war_and_peace.txt";
	struct stat statbuf;
	memset(&statbuf,0,sizeof(statbuf));
	stat(readfile,&statbuf);
	long len = statbuf.st_size;
	return len;
} 
void * handle_client(void * arg){ //void* arg本来就是int强转而来
	printf("pid: %lu\n",pthread_self());
	
	struct train *ptrain = (struct train*)arg;
	char* filename = "../war_and_peace.txt";
	size_t len = strlen(filename);
	if(send(ptrain->_newfd,filename,len,0)==-1){
		perror("send filename failed\n");
	}
	printf("filename send ok  ");
	send(ptrain->_newfd,(void*)&ptrain->_start,sizeof(long),0);				//& 取地址符优先级低
	send(ptrain->_newfd,(void*)&ptrain->_end,sizeof(long),0);
	printf("start,end send ok\n");
	int table[26];
	recv(ptrain->_newfd,table,26*sizeof(int),0);
	
	pthread_mutex_lock(&output_lock);
	for(int i=0;i<26;i++)
		ans[i] += table[i];
	pthread_mutex_unlock(&output_lock);
	
	close(ptrain->_newfd);
	//return NULL;								//有什么区别？
	pthread_exit(NULL);							//线程主动终止
}

int main(int argc, const char *argv[])
{
	struct sockaddr_in server, client;
	int sfd,newfd;
	
	// Create socket
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Could not create socket");
		return -1;
    }
    printf("Socket created");
	// Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;  			//自动填充本机IP 地址. 0.0.0.0
    server.sin_port = htons(8888);
    // Bind
    if (bind(sfd,(struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed. Error");
        return -1;
    }
    printf("bind done");
    // Listen
    listen(sfd, 3);
	
	int IPnum = countIP();
	pthread_t cli_thread[IPnum] ;
	bzero(cli_thread,sizeof(pthread_t)*IPnum);
	struct train t[IPnum];    				//保证了传递给线程函数后 数据内容不会即刻销毁
	pthread_mutex_init(&output_lock, NULL);
	
	for(int index=0;index<IPnum;index++){
		int socksize = sizeof(struct sockaddr_in);
		if( (newfd = accept(sfd,(struct sockaddr*)&client,(socklen_t *)&socksize)) == -1 ){			//accept是阻塞函数
			perror("accept failed");
			return 1;
		}
		printf("\n");
		printf("----worker %d----\n",index);
		printf("client ip=%s , port=%d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
		
		//生成一个线程来完成和客户端的会话，父进程继续监听
		
			bzero(&t[index],sizeof(struct train));
			t[index]._newfd = newfd;
			t[index]._start = startFile(index,IPnum);
			if(index!=IPnum-1)
				t[index]._end = endFile(index,IPnum);
			else
				t[index]._end = endFileFinal();
			printf("intdex:%d start:%ld end:%ld\n",index,t[index]._start,t[index]._end);				
			pthread_create(&cli_thread[index],NULL,handle_client,(void *)&t[index]);
			printf("----pid=%lu: thread doing\n",cli_thread[index]);		
		
	}
	
	int retvalue=0;
	//如果没有这句话 那么最后一个线程尚未执行完毕
	pthread_join(cli_thread[IPnum-1],(void**)&retvalue);				//等待最后一个线程退出 获取返回值; join阻塞函数  
	
	for(int k=0;k<26;k++)
		printf("%c = %d\n",'a'+k,ans[k]);
	close(sfd);
	return 0;
}

#if 0
注意整数的网络、主机字节序的转换 
如果我全部使用主机字节序 发送解析似乎也没有问题？毕竟是自己测试
但是为了跨平台的问题 所以还是需要解决字节序的问题(因为别人与你的主机的字节序并不相同)
#endif  