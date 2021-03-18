#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int getint(char* str){
	int value=0;
	
	for(int i=0; str[i]!='0'; i++) {
		printf("%d\n",str[i]);
		if(str[i]<48||str[i]>57)return -1;
		else value=value*10+str[i];}
	return value;
}



int main(int argc , char** argv){
	struct sockaddr_in server_addr, client_addr;
	char buff[256];

	//get port number, if no port # input print error message & exit	
	if(argc<2){printf("Error: few arguments.\n"); return -1;}
	int port=atoi(argv[1]);
	
	if(port==0){ printf("Error: port invalid.\n"); return -1;}

	//create UDP socket, get socket file descriptor, print Error message if failed.
	int s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	if(s==-1){ printf("Error: socket fail.\n"); return -1;}

	//initialize server_addr
	bzero((char*) &server_addr, sizeof(server_addr));
	
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	server_addr.sin_port=htons(port);
	
	
	
	int b=bind(s,(struct sockaddr*)&server_addr, sizeof(server_addr));
	if(b==-1) {perror("bind"); return -1;}
	
	//start listen
	//int l=listen(s, 5);
	//if(l==-1) {perror("listen"); return -1;}

	int cli=sizeof(client_addr);
	//int newsocket=accept(s, (struct sockaddr*)&client_addr, &cli);
	//if(newsocket==-1){printf("Error: accept fail.\n"); return -1;}
	//, &client_addr, &cli
	int r=recvfrom(s,&buff, 255,0, &client_addr, &cli);
	while(r==0)r=recvfrom(s,&buff, 255,0, &client_addr, &cli);
	if(r==-1){perror("Error: read fail.\n"); return -1;}
	printf("%s\n",buff);
	int w;
	
	if(strcmp(buff, "ftp")==0) 
		w=sendto(s,"yes", sizeof("yes"), 0,&client_addr, cli);		
	else  w=sendto(s,"no", sizeof("no"),0, &client_addr, cli);
	if(w==-1) {perror("Error: write fail.\n"); return -1;}
	return 0;

}
