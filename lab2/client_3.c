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
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
//#include "packet.h"
#define SERVERPORT "4950"	// the port users will be connecting to

struct packet { 
	unsigned int total_frag;
 	unsigned int frag_no; 
	unsigned int size; 
	char* filename; 
	char filedata[1000]; 
};


void sendFile(char* filename, int sockfd, struct sockaddr server_addr);

int vaildString(char * str);

int recvtimeout(int __fd, void *__restrict __buf, size_t __n,
			 int __flags, __SOCKADDR_ARG __addr,
			 socklen_t *__restrict __addr_len, int  timeout){
    fd_set fds;
    int n;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(__fd, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = timeout;
    n = select(__fd+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error
    return recvfrom(__fd, __buf, __n, __flags, __addr, __addr_len);
}


char* packtoString(struct packet * pack){

	int size=sizeof(unsigned)*3+sizeof(pack->filename)+sizeof(pack->filedata)+4*sizeof(':')+1;
	char* string=(char*)malloc(size);
	int loc=0;
	sprintf(string+loc,"%u:",pack->total_frag);
	loc=strlen(string);
	sprintf(string+loc,"%u:",pack->frag_no);
	loc=strlen(string);
	sprintf(string+loc,"%u:",pack->size);
	loc=strlen(string);
	sprintf(string+loc,"%s:", pack->filename);
	loc=strlen(string);
	memcpy(string+loc, pack->filedata, pack->size);
	//*(string+loc+pack->size)='\0';
	return string;


}

struct packet* stringtoPack(char* str){
	struct packet* a= malloc(sizeof(struct packet));
	char* ptr;
	a->total_frag= (unsigned int)strtoul(str, &ptr, 10);
	a->frag_no=(unsigned int)strtoul(ptr+sizeof(char), &ptr, 10);
	a->size=(unsigned int)strtoul(ptr+sizeof(char), &ptr, 10);

	char* e=strchr(ptr+1,':');
	a->filename=malloc(e-ptr);
	memcpy(a->filename,ptr+1, e-ptr-1);
	a->filename[e-ptr-1]='\0';	
	ptr=e;
	memcpy(a->filedata,ptr+1, a->size);
	
	
	return a;

}


void printPack(struct packet* pack){

	printf("%u:%u:%u\n",pack->total_frag,pack->frag_no,pack->size);
	printf("%s\n",pack->filename);
	printf("%s\n",pack->filedata);

}



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

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
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
        
	
	sendFile(file,sockfd,*p->ai_addr);

        freeaddrinfo(servinfo);
        
	close(sockfd);

	return 0;
}



void sendFile(char* filename, int sockfd, struct sockaddr server_addr){
	
	/*get file stream and get size and total frag*/
	FILE* f=fopen(filename, "r");
	if(f==NULL) {fprintf(stderr,"Open file error.\n");exit(1);}
	
	fseek(f,0,SEEK_END);
	unsigned int size=ftell(f);
	rewind(f);
	unsigned int total_frag=(size%1000==0)?size/1000+0:size/1000+1;
	printf("%u tf %u\n", size, total_frag );
	/*allocate array for packets and initialize*/
	struct packet** p_array=calloc(total_frag, sizeof(struct packet*));
	for(int i=0; i<total_frag;i++){
		p_array[i]=malloc(sizeof(struct packet));
		p_array[i]->total_frag=total_frag;
		p_array[i]->frag_no=i+1;  		/*# from 1 to total frag*/
		p_array[i]->size=(i==(total_frag-1))?size%1000:1000;
		p_array[i]->filename=filename;				/* All link to given filename*/
		fread(p_array[i]->filedata, 1, p_array[i]->size, f); /*read from file stream*/
		//fseek(f, p_array[i]->size, SEEK_CUR);                /*offset by size */
	}
	
	/*set time limit*/
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
        clock_t start ,end;
	
	/*set time limit to socket */
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        	fprintf(stderr, "setsockopt failed\n");
	}
	
	//int timesent = 0;   // Number of times a packet is sent

	/*create two var for interacting with server*/
    	socklen_t server_addr_size = sizeof(server_addr);
	char rec_buf[1100];
	memset(rec_buf, 0, sizeof(char) * 1100);
	/*send package as string, wait for responce ACK till timeout, if so send next, else resend after TO*/
	//int sent=0;
        for(int i=0; i<total_frag;i++){
            //if(sent==10){printf("too many resend on %d, transmission fail.\n",i);break;} 
            char* str=packtoString(p_array[i]);
            if(sendto(sockfd, str,1100,0, (struct sockaddr *) &server_addr, sizeof(server_addr))==-1){
                    fprintf(stderr,"send to error packet %d.\n", i+1);
                    exit(1);
            }
//            if(recvfrom(sockfd, rec_buf, 1100, 0 , (struct sockaddr *) &server_addr, &server_addr_size)==-1){
//                    fprintf(stderr, "ACK timeout for %d.\n", i+1);			
//            }
            start = clock();
            int n = recvtimeout(sockfd, rec_buf, 1100, 0 , (struct sockaddr *) &server_addr, &server_addr_size,10);
            end = clock();
            //printf("%Lf\n", (long double)(end - start));
            if(n == -1){
                perror("recvtimeout\n");
            }
            
            else if(n == -2){
                printf("timeout resend!!!\n");
                i--;
				//sent++;
                //break;
            }
            else{
                struct packet* pack=NULL;
                if(rec_buf[0]!='\0')
                        if(vaildString(rec_buf))
                                pack=stringtoPack(rec_buf);
                        else {printf("rec not valid\n"); exit(1);}
                //printf("123\n");
                memset(rec_buf, 0, sizeof(char) * 1100);
                
                if(pack==NULL);
                //printf("pack data: %c %c %c", )
                else if(!(pack->filedata[0]=='A'&&pack->filedata[1]=='C'&&pack->filedata[2]=='K')) {
					if(pack->filedata[0]=='N'&&pack->filedata[1]=='A'&&pack->filedata[2]=='C'&&pack->filedata[3]=='K'){
						//printf("send too fast, rec frag:%d, curr: %d\n",pack->frag_no,i);
						if(pack->frag_no-1<=i){ printf("send too fast, rec frag:%d, curr: %d\n",pack->frag_no,i);i=pack->frag_no-2; }
						else {fprintf(stderr, "not ACK %d, resend\n",i); i--;/*sent++;*/}
					}
					


				}
                else printf("packet number %d\n",i);
                if(pack!=NULL){
                        free(pack->filename);		
                        free(pack);
                }
                free(str);
            }

        }
        //finish clear data
        printf("cleaning\n");
        for(int i=0; i<total_frag;i++) free(p_array[i]);

	


}

int vaildString(char * str){

	int num=0;
	int i=0;
	for(int i=0; i<strlen(str);i++){
		
		if(str[i]==':') num++;
	}
	if(num>=4) return 1;
	else return 0;


}
