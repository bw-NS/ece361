/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVERPORT "4950"	// the port users will be connecting to

void sendFile(char* filename, int sockfd, struct sockaddr_in server_addr);


int main(int argc, char *argv[])
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
        char message[10];
        char file[100];
        char buf[1000];

	if (argc != 3) {
		fprintf(stderr,"usage: talker hostname message\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}
	
        
        scanf("%s %s",message,file);
        
        if (strcmp(message,"ftp") != 0){
            perror("message wrong.\n");
            exit(1);
        }
        if (access(file,F_OK) == -1){
            printf("file %s does not exist.", file);
            exit(1);
        }
        
        int check = sendto(sockfd, "ftp", sizeof("ftp"), 0, p->ai_addr, (p->ai_addrlen));
        if (check == -1){
            fprintf(stderr, "sendto error\n");
            exit(1);
        }
        
        if ((numbytes = recvfrom(sockfd, buf, 1000 - 1, 0, p->ai_addr, &(p->ai_addrlen))) == -1) {
            perror("recvfrom");
            exit(1);
        }
        
        if (strcmp(buf,"yes") != 0) exit(1);
        printf("A file transfer can start.\n");
        
        freeaddrinfo(servinfo);
        
	close(sockfd);

	return 0;
}



void sendFile(char* filename, int sockfd, struct sockaddr_in server_addr){

	FILE* f=fopen(filename, "r");
	if(f==NULL) {fprintf(stderr,"Open file error.\n");exit(1);}
	
	fseek(f,0,SEEK_END);
	unsigned int size=ftell(f);
	rewind(f);
	unsigned int total_frag=size/1000+size%1000?1:0;
	
	//allocate array for packets
	struct packet** p_array=calloc(total_frag, sizeof(struct packet*));
	for(int i=0; i<total_frag;i++){
		p_array[i]=malloc(struct packet);
		p_array[i]->total_frag=total_frag;
		p_array[i]->frag_no=i+1;
		(i==(total_frag-1))?p_array[i]->size=size%1000:p_array[i]->size=1000;
		p_array[i]->filename=filename;
		
	}
	



}



