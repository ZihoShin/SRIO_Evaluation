/******************************************************/
/********** TEST CODE Modified by Ziho Shin ***********/
/******************************************************/
/******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#define MB			262144
//////////////////////////////////////////////////////

void error_handling(char *message);

void main(int argc, char **argv){
        int sock;
	char msg[MB];
	int str_len;
        char sync[]="Sync";
	char ack[] ="ack";
	int fd;

        struct sockaddr_in serv_addr;

        if(argc != 3){
                printf("Usage : %s <IP> <port>\n", argv[0]);
                exit(1);
        }

        printf("\n\n================================================ \n");
        printf("================================================ \n");
        printf("Ethernet Networking Test Program - Client\n");
        printf("================================================ \n");
        printf("================================================ \n\n");
        /******************************************************/

        sock = socket(PF_INET, SOCK_STREAM, 0);
	//flag = fcntl(sock, F_GETFL, 0);
	//fcntl(sock,F_SETFL, flag|O_NONBLOCK);//socket non-blocking mode
        //Socket creation for connecting to server
        if(sock == -1)
                error_handling("Socket create error");

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
        serv_addr.sin_port = htons(atoi(argv[2]));

        if(connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1 ) // Connect to server
		error_handling("Server Connection error!");
/////////////////////////////////////////////////////////////////////////        
	printf("Server connected Information :\n\n");
        printf("Port Number                  : %s\n", argv[2]);
        printf("Server address               : %s\n", argv[1]);
        printf("================================================ \n");
        printf("Writing Sync message to Server\n");
	write(sock, sync, sizeof(sync)); //sync sign
	read(sock, msg, sizeof(msg)-1);//echo back
	printf("%s\n",msg);
        printf("Ready to receive Dummy data\n");
        printf("Start to transfer\n");
	fd = open("received_eth.dat", O_WRONLY|O_CREAT|O_TRUNC);
	if(fd == -1)
                error_handling("File open error");

	while((str_len = read(sock, msg, MB))!=0){
                write(fd, msg, str_len);
        }
//	write(sock, ack, sizeof(ack));
        close(fd);
//	write(sock, ack, sizeof(ack));
/*
	for(loop1 = 0; loop1<loop2; loop1++){
			for(loop3 = 0; loop3<MB; loop3++){
				read(sock, (int*)&message[loop3],sizeof(msg)-1);
				count = count+1;
				if(count == MB*loop2) write(sock, ack, sizeof(ack));
				else write(sock, sync, sizeof(sync));
			}
	}
*/
	close(sock); // connection close
        printf("================================================ \n");
        printf("TCP over dummy data transfer finished\n");
        printf("Socket Connection closed\n");
//	close(sock);
        printf("Received dummy data saved as 'received_eth.dat'\n");
        printf("\n");
}
	

void error_handling(char *message){
        fputs(message, stderr);
        fputc('\n', stderr);
        exit(1);
}

