#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char** argv){


	struct sockaddr_in server_addr;
	int s;
	char type[100] , file[100];
	char buff[256];
	printf("Input from user: \n");
   	scanf("%s",type );
	scanf("%s",file);
	bzero(&server_addr, sizeof(server_addr));
	if(argc>=3){
		server_addr.sin_family=AF_INET;
		server_addr.sin_port=htons(atoi(argv[2]));
		inet_pton(AF_INET,argv[1], (struct in_addr*) &server_addr.sin_addr);
		s=socket(AF_INET, SOCK_DGRAM, 0);
		if(s==-1) perror("socket");
	}
	int se=sendto(s, type, sizeof(type), 0 , (struct sockaddr*) &server_addr, sizeof(server_addr) );
	if(se==-1) perror("send ");
	int serlen=sizeof(server_addr);
	int rec=recvfrom(s, &buff,sizeof(buff), 0, (struct sockaddr*) &server_addr, &serlen );
	while(rec==0){
		rec=recvfrom(s, &buff,sizeof(buff), 0, (struct sockaddr*) &server_addr, &serlen );
	}
	if(rec==-1) perror("recv");
	
	else{ printf("%s\n",buff); 
		if (strcmp(buff,"yes")==0)printf("A file transfer can start\n");
	} 
	return 0;

}
