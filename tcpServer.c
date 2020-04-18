#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h> 
 
#define SERVPORT 3333
#define BACKLOG 10
#define MAX_CONNECTED_NO 10
#define MAXDATASIZE 1500
 
void *tcp_client_thread(void *arg)
{
   	int i,rec_len;
    	int mcfd = *((int*)(arg));
	char buf[100];
	printf("tcp_client_thread start client fd = %d\r\n",mcfd);
	while(1)
	{
		if((rec_len = recv(mcfd, buf, MAXDATASIZE, 0)) == -1) {
			 printf("tcp_client_thread revc err client fd = %d\r\n",mcfd);
			 close(mcfd);
			 return NULL;
		}
		else if(rec_len > 0){
			printf("recv from fd=%d: ",mcfd);
			for(i=0;i<rec_len;i++)
			{
				printf("%c",buf[i]);
			}
			printf("\r\n");
		}
	}
	
}
 
 
int main() {
    struct sockaddr_in server_sockaddr, client_sockaddr;
    int sin_size, recvbytes;
    int sockfd, client_fd;
    //char buf[MAXDATASIZE];
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    printf("socket success! sockfd = %d\n", sockfd);
    
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(SERVPORT);
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_sockaddr.sin_zero), 8);
    
    if(bind(sockfd, (struct sockaddr *)&server_sockaddr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }
    printf("bind success!\n");
    
    if(listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    printf("listening!\n");
    
	while(1)
	{
		int err,cfd;
		pthread_t pt;
		if((client_fd = accept(sockfd, (struct sockaddr *)&client_sockaddr, &sin_size)) == -1) {
			perror("accept");
			//exit(1);
		}else {
			printf("client connet success\r\n");
			cfd = client_fd;
			err = pthread_create(&pt, NULL, &tcp_client_thread, &cfd);
			if(err != 0) 
				printf("\n can't create thread:[%s]", strerror(err));
			else
				printf("\n Thread created successfully\n");
		}
		
		//cout << "received a connection: " << buf << endl;
		//close(sockfd);
	}
    return 0;
}




