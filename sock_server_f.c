/******************************************************/
/********** TEST CODE Modified by Ziho Shin ***********/
/******************************************************/
/******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#define MB			262144
////////////////////////////////////////////////////
int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1){
    long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;
 
    return (diff<0);
}
void error_handling(char *message);

void main(int argc, char **argv){
        int serv_sock;
	int clnt_sock;
	int clnt_addr_size;
	int fd;
	int str_len;
	char sync[MB];
	char msg[MB];
        struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
/******************************************************/
        clock_t check;
        struct timeval tvBegin, tvEnd, tvDiff;
//       struct timespec start, stop;
//       double ExeTime, ExeTime_nano;
        char buf[100];
/******************************************************/
        printf("\n\n================================================ \n");
        printf("================================================ \n");
        printf("Ethernet Networking Test Program - Server\n");
        printf("================================================ \n");
        printf("================================================ \n\n");
        /******************************************************/
        /******************************************************/
        printf("\n");
        check = time(NULL);
        strftime(buf,sizeof(buf), "Program_Started: %a %Y-%m-%d %H:%M:%S location: %Z", localtime(&check));
        printf("%s\n\n", buf);
        /******************************************************/
	if(argc!=2){
                printf("Usage : %s <port>\n", argv[0]);
                exit(1);
        }

        serv_sock = socket(PF_INET, SOCK_STREAM, 0);
        //Sever socket create
        if(serv_sock == -1)
                error_handling("Socket create error");

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        //Server IP adderess
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //RX from anywhere
        serv_addr.sin_port=htons(atoi(argv[1]));
        //Server port number

        if(bind(serv_sock, (struct sockaddr*) & serv_addr,
                sizeof(serv_addr)) == -1) // Socket address assign
                error_handling("Socket binding error");

        if(listen(serv_sock, 5) == -1) // Connection waiting
                error_handling("Server listen error");

        clnt_addr_size = sizeof(clnt_addr);

        clnt_sock = accept(serv_sock, (struct sockaddr*) & clnt_addr,
                        & clnt_addr_size); // Connection accepting
/////////////////////////////////////////////////////////////////////////
        printf("Client connected Information :\n\n");
	printf("Port Number                  : %s\n", argv[1]);
	printf("Client address               : %s\n", inet_ntoa(clnt_addr.sin_addr));
	printf("================================================ \n");
        if(clnt_sock == -1){
                error_handling("Accept error");
	}
	printf("Waiting for Client Sync message\n");
	str_len = read(clnt_sock, sync, MB);
	printf("%s\n",sync);
	write(clnt_sock, sync, str_len); //echo back
	printf("Ready to send Dummy data\n");
	printf("Start to transfer\n");
	fd = open("dummy.db", O_RDONLY);
        if(fd == -1)
                error_handling("file open error");
//////////////////////////////////////////////////////////////////////////////////////////
//        clock_gettime(CLOCK_MONOTONIC, &start);
         gettimeofday(&tvBegin, NULL);
//////////////////////////////////////////////////////////////////////////////////////////
       while((str_len = read(fd, msg, MB))!=0){
                write(clnt_sock, msg, str_len);
        }
//	read(clnt_sock, sync,MB);
//	printf("%s\n",sync); 
        close(fd);
//////////////////////////////////////////////////////////////////////////////////////////
	gettimeofday(&tvEnd, NULL);
        timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
        printf("ExeTime : %ld.%06ld s\n", tvDiff.tv_sec, tvDiff.tv_usec);
        printf("ExeTime : %06ld us\n",tvDiff.tv_usec);
//	clock_gettime(CLOCK_MONOTONIC, &stop);
//        ExeTime_nano = (((long)stop.tv_nsec)-((long)start.tv_nsec));
       //ExeTime_nano = ((stop.tv_sec-start.tv_sec));
//        ExeTime = ExeTime_nano * (0.00001);
//        printf("\nEXE Time_n: %f nsec\n", ExeTime_nano);
//        printf("EXE Time_m: %f msec\n\n", ExeTime);
//////////////////////////////////////////////////////////////////////////////////////////
//	read(clnt_sock, sync, MB);
//	printf("%s\n", sync);
/*
	while(1){
	write(clnt_sock, (char*)&message[0],sizeof(int));
	read(clnt_sock, sync, MB);
	if(!strcmp(sync,"ack")) break;
	}
*/
        close(clnt_sock); // Connection close
        printf("================================================ \n");
	printf("TCP over dummy data transfer finished\n");
	printf("Socket Connection closed\n");
	printf("\n");
	check = time(NULL);
        strftime(buf,sizeof(buf), "Program_Ended: %a %Y-%m-%d %H:%M:%S location: %Z", localtime(&check));
        printf("%s\n\n", buf);
}

void error_handling(char *message){
        fputs(message, stderr);
        fputc('\n', stderr);
        exit(1);
}

