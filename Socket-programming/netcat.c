#include <arpa/inet.h> 
#include <errno.h> 
#include <netinet/in.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <strings.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#define MAXLINE 1024 

    int listenfd, connfd, udpfd, nready, maxfdp1,sockfd; 
    char buffer[MAXLINE]; 
    pid_t childpid; 
    fd_set rset; 
    ssize_t n; 
    socklen_t len; 
    const int on = 1; 
    struct sockaddr_in cliaddr, servaddr; 
    char* message = ""; 
    void sig_chld(int); 
    int Keep_Alive=1;

int max(int x, int y) 
{ 
    if (x > y) 
        return x; 
    else
        return y; 
} 

//--------------SERVER---------------
void response(){
char message2[1024];
while(Keep_Alive){
fgets(message2,MAXLINE,stdin);
write(connfd, (const char*)message2, sizeof(buffer));
sendto(sockfd, (const char*)message2, strlen(message2), 
           0, (const struct sockaddr*)&servaddr, 
           sizeof(servaddr)); 
}
}

void call_response(){
	pthread_t response_thread;
    if (pthread_create(&response_thread, NULL, (void *) response, NULL) != 0) {
        printf ("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }
}

//-------------SERVER TCP--------------
void tcp(){
while(Keep_Alive){
FD_SET(listenfd, &rset); 
if (FD_ISSET(listenfd, &rset)) { 
            len = sizeof(cliaddr); 
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len); 
            if ((childpid = fork()) == 0) { 
while(Keep_Alive){
                bzero(buffer, sizeof(buffer)); 
                read(connfd, buffer, sizeof(buffer)); 
                puts(buffer); 
		message = "";
                write(connfd, (const char*)message, sizeof(buffer)); 
};
            } 
        } 
}
}


void call_tcp(){
	pthread_t tcp_thread;
    if (pthread_create(&tcp_thread, NULL, (void *) tcp, NULL) != 0) {
        printf ("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }
}


//---------------SERVER UDP-----------
void udp(){
while(Keep_Alive){
FD_SET(udpfd, &rset); 
if (FD_ISSET(udpfd, &rset)) { 
            len = sizeof(cliaddr); 
            bzero(buffer, sizeof(buffer)); 
            n = recvfrom(udpfd, buffer, sizeof(buffer), 0, 
                         (struct sockaddr*)&cliaddr, &len); 
            puts(buffer); 
            sendto(udpfd, (const char*)message, sizeof(buffer), 0, 
                   (struct sockaddr*)&cliaddr, sizeof(cliaddr)); 
        } 
}
}

void call_udp(){
pthread_t udp_thread;
    if (pthread_create(&udp_thread, NULL, (void *) udp, NULL) != 0) {
        printf ("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }
}

//--------------SERVER TCP/UDP SOCKETS---------------
void server(int argc, char *argv[]){
int reuseaddr = 1;
int PORT = atoi(argv[2]);

    listenfd = socket(AF_INET, SOCK_STREAM, 0); 
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 


    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
    listen(listenfd, 10); 
  

    udpfd = socket(AF_INET, SOCK_DGRAM, 0); 

    bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
  

    FD_ZERO(&rset); 
  
    // get maxfd 
    maxfdp1 = max(listenfd, udpfd) + 1; 
}

//---------------CLIENT TCP-----------------
void write_tcp_4(){
char message2[1024];

while(Keep_Alive){
	memset(buffer, 0, sizeof(buffer)); 
    fgets(message2, MAXLINE,stdin);
    strcpy(buffer, message2); 
	write(sockfd, buffer, sizeof(buffer));


}
}

void call_write_tcp_4(){
pthread_t write_tcp_4_thread;
    if (pthread_create(&write_tcp_4_thread, NULL, (void *) write_tcp_4, NULL) != 0) {
        printf ("Create pthread error!\n");
        exit(EXIT_FAILURE);
}
}

void client_tcp_4(int argc, char *argv[],int ip,int port){
int PORT = atoi(argv[port]);
if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("socket creation failed"); 
        exit(0); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
  
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = inet_addr(argv[ip]); 
  
    if (connect(sockfd, (struct sockaddr*)&servaddr,  
                             sizeof(servaddr)) < 0) { 
        printf("\n Error : Connect Failed \n"); 
    }
call_write_tcp_4();
while(Keep_Alive){
    read(sockfd, buffer, sizeof(buffer)); 
	if(strncmp(buffer,"/term",5) != 0)
    puts(buffer);
	else end();
} 
}


//---------CLIENT UDP---------
void write_udp(){
char message2[1024];

while(Keep_Alive){
	memset(buffer, 0, sizeof(buffer)); 
    fgets(message2, MAXLINE,stdin);
	sendto(sockfd, (const char*)message2, strlen(message2), 
           0, (const struct sockaddr*)&servaddr, 
           sizeof(servaddr)); 


}
}

void call_write_udp(){
pthread_t write_udp_thread;
    if (pthread_create(&write_udp_thread, NULL, (void *) write_udp, NULL) != 0) {
        printf ("Create pthread error!\n");
        exit(EXIT_FAILURE);
}
}

void client_udp(int argc, char *argv[]){
int PORT = atoi(argv[3]);
if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("socket creation failed"); 
        exit(0); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
  
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = inet_addr(argv[2]); 
  
    if (connect(sockfd, (struct sockaddr*)&servaddr,  
                             sizeof(servaddr)) < 0) { 
        printf("\n Error : Connect Failed \n"); 
    }
call_write_udp();
while(Keep_Alive){
    n = recvfrom(sockfd, (char*)buffer, MAXLINE, 
                 0, (struct sockaddr*)&servaddr, 
                 &len); 
	if(strncmp(buffer,"/term",5) != 0)
    puts(buffer); 
else end();
} 
}
void end(){
char message2[5] = "/term";
write(connfd, (const char*)message2, sizeof(buffer)); 
	Keep_Alive=0;
	close(sockfd);  
	close(connfd); 
	close(listenfd); 
	close(udpfd);
	int pthread_cancel(pthread_t write_udp_thread);
	int pthread_cancel(pthread_t write_tcp_4_thread);
	int pthread_cancel(pthread_t udp_thread);
	int pthread_cancel(pthread_t tcp_4_thread);
	system("clear");
	int kill(getpid,TERM);
}
//-----------MAIN--------------
int main(int argc, char *argv[]) 
{ 

	signal(SIGINT,end);


	if(strncmp(argv[1],"-l",2) == 0){
printf("-l\n");
	server(argc,argv);
	call_tcp();
	call_udp();
	call_response();
}
	else if(strncmp(argv[1],"-4",2) == 0){
	client_tcp_4(argc,argv,2,3);
}
else if(strncmp(argv[1],"-u",2) == 0){
	client_udp(argc,argv);
}
else
	client_tcp_4(argc,argv,1,2);

while(Keep_Alive){};
printf("Killed\n");
} 
