#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/types.h> //头文件
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

typedef struct{
	int len;
	char buf[1000];
}train,*ptrain;
void calculate(int table[26],long start,long end,char* filename);
 
int main(int argc, char *argv[])
{
	char buf[1000];
    int sfd;
	//Create socket
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        printf("Could not create socket");
    }
    printf("Socket created");
    
	//设置 server IP+port
	struct sockaddr_in server;
	bzero(&server,sizeof(struct sockaddr_in));
    server.sin_addr.s_addr = inet_addr("10.0.0.1");   				//remote server IP地址
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );
	
	//Connect to remote server  这里没有使用本地端口,将由协议栈自动分配一个端口
    if (connect(sfd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("connect failed. Error");
        return 1;
    }
    printf("Connected\n");
	
	//receive task from server
	// int len;
	// recv_n(sfd,(char*)&len,sizeof(int));
	bzero(buf,sizeof(1000));
	recv(sfd,buf,sizeof(buf),0);
	printf("filename:%s",buf);
	
	long start=0,end=0;
	recv(sfd,&start,sizeof(long),0);
	recv(sfd,&end,sizeof(long),0);
	printf("   start:%ld end:%ld\n",start,end);
	
	int table[26]={0};
	calculate(table,start,end,buf);
	for(int i=0;i<26;i++)
		printf("%d ",table[i]);
	printf("\n");
	
	send(sfd,table,sizeof(int)*26,0);
	printf("send hashtable ok\n");
	
	close(sfd);
	return 0;
}
	
void calculate(int table[26],long start,long end,char* filename){
	char buf[1000];
	bzero(buf,sizeof(char)*1000);
	int fd = open(filename,O_RDONLY);
	lseek(fd,start,SEEK_SET);							//偏移到start位置	
														//计算对于 [start,end]所有内容 需要read到buf中多少次
	int readnum = (end-start)/1000;
	for(int num=0;num<readnum;num++){					//函数 pread(int fd, void *buf, size_t count, off_t offset); = lseek+read相当于新的原子操作 
														//http://blog.csdn.net/zongcai249/article/details/17598411
		read(fd,buf,sizeof(char)*1000);
		for(int i=0;i<strlen(buf);i++){
			if(buf[i]>='A' && buf[i]<='Z')
				table[buf[i]-'A'] ++;
			else if(buf[i]>='a' && buf[i]<='z')
				table[buf[i]-'a'] ++;
		}
		bzero(buf,sizeof(char)*1000);
	}
	int left_stay = end - start - readnum*1000;
	if( left_stay > 0){
		read(fd,buf,left_stay);
		for(int i=0;i<strlen(buf);i++){
			if(buf[i]>='A' && buf[i]<='Z')
				table[buf[i]-'A'] ++;
			else if(buf[i]>='a' && buf[i]<='z')
				table[buf[i]-'a'] ++;
		}
		bzero(buf,sizeof(char)*1000);
	}
}