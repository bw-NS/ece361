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
#include <stdbool.h>
//#include "packet.h"

#define MYPORT "4950" // the port users will be connecting to

#define MAXBUFLEN 1100

// get sockaddr, IPv4 or IPv6:
struct packet { 
	unsigned int total_frag;
 	unsigned int frag_no; 
	unsigned int size; 
	char* filename; 
	char filedata[1000]; 
};
//char* packtoString(struct packet *pack);
//struct packet* stringtoPack(char* str);
//void printPack(struct packet* pack);

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
	for(int i=0; i<pack->size;i++){
		printf("%d\n",*(pack->filedata+i));
	}
	printf("%s\n",pack->filedata);

}


void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char const *argv[]) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    bool signal = true;
    char port[100];
    char server[100];

    printf("server <UDP listen port>: ");
    
    char *response = "yes";
    
    //printf("%s\n",response);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
            (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    
    //char *response = "yes";
    printf(" this is buf %s\n",buf);
    if (strcmp(buf, "ftp") == 0) {
        printf("hhihi");
        int check = sendto(sockfd, "yes", sizeof("yes"), 0, (struct sockaddr *) &their_addr, addr_len);
        if (check == -1) {
            fprintf(stderr, "sendto error\n");
            exit(1);
        }
    } 
    else {
        int check = sendto(sockfd, "no", sizeof("no"), 0, (struct sockaddr *) &their_addr, addr_len);
        if (check == -1) {
            fprintf(stderr, "sendto error\n");
            exit(1);
        }
    }
    
    printf("listener: got packet from %s\n",
            inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *) &their_addr),
            s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);
    
    printf("i am here!!!\n");
    
    struct packet* localpacket = (struct packet *) malloc(sizeof(struct packet));
    FILE *receive_file_name = NULL;
    //bool *check_recv = NULL;
    char* filename;
    char* send_back_data;
    int check_number = 1;
    printf("i am in 184!!!\n");
    
//    if (recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *) &their_addr, &addr_len) == -1){
//            perror("recvfrom");
//            exit(1);
//        }
//    printf("%s\n",buf);
//    
//    localpacket = stringtoPack(buf);
//    printf("filename: %s\n",localpacket->filename);
//    printf("total: %d\n",localpacket->total_frag);
    
    
    while(true){
        printf("I am in translation!!!!!!!!!!!!!!\n");
        
        //check if receive or not
        if (recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *) &their_addr, &addr_len) == -1){
            perror("recvfrom");
            exit(1);
        }
        printf("%s\n",buf);
        //break;
        
        //convert receive string to packet.
        localpacket = stringtoPack(buf);
        
        printf("filename: %s\n",localpacket->filename);
        printf("total: %d\n",localpacket->total_frag);
        //printf("this is data: %s\n",localpacket->filedata);
        printf("this is data: ");
        for (int i =0; i < localpacket->size; i++){
            printf("%c",(localpacket->filedata)[i]);
        }
        
        //break;
        
        
        
        
        
        //create a file named 
        if (!receive_file_name){
            filename = localpacket->filename;
            receive_file_name = fopen(filename,"ab+");
        }
        printf("I am in 202!!!!!!!!!!!!!!\n");
        printf("this is check_number %d\n", check_number);
        //create a list of boolean to each packet,initial with false.
//        if (!check_recv){
//            check_recv = (bool *)malloc(localpacket->total_frag * sizeof(bool));
//            for (int i = 0; i < localpacket->total_frag; i++){
//                check_recv[i] = false;
//            }
//        }
        
        if(check_number == localpacket->frag_no){
            printf("I am in 212!!!!!!!!!!!!!!\n");
            fprintf(receive_file_name,"%s",localpacket->filedata);
            //fputs(localpacket->filedata, receive_file_name);
//            (localpacket->filedata)[0] = 'A';
//            (localpacket->filedata)[1] = 'C';
//            (localpacket->filedata)[2] = 'K';
//            (localpacket->filedata)[3] = '\0';
//            printf("this is ACK content: %s",localpacket->filedata[0]);
//            printf("this is ACK content: %s",localpacket->filedata[1]);
//            printf("this is ACK content: %s",localpacket->filedata[2]);
//            printf("this is ACK content: %s",localpacket->filedata[3]);
            strcpy(localpacket->filedata,"ACK\0");
            
            localpacket->size = strlen("ack\0");
            
            send_back_data = packtoString(localpacket);
            printf("back_data: %s\n",send_back_data);
            printf("back_data: %s\n",localpacket->filedata);
            printf("back_data: %s\n",localpacket->filename);
            //printf("back_data: %s\n",strlen(temp));
            
            if ((sendto(sockfd, send_back_data, sizeof(send_back_data), 0, (struct sockaddr *) &their_addr, addr_len))== -1) {
                fprintf(stderr, "sendto error\n");
                exit(1);
            }
            
            if(check_number == localpacket->total_frag){
                printf("File %s transfer completed",filename);
                break;
            }
            
            check_number++;
        }
        
        if(check_number != localpacket->frag_no){
            strcpy(localpacket->filedata,"NACK");
            
            localpacket->size = strlen("NACK");
            
            send_back_data = packtoString(localpacket);
            
            if ((sendto(sockfd, send_back_data, sizeof(send_back_data), 0, (struct sockaddr *) &their_addr, addr_len))== -1) {
                fprintf(stderr, "sendto error\n");
                exit(1);
            }
        }

//        if (!check_recv[localpacket->frag_no]){//if not review before, set corresponding bool to true, and store this packet into packet_list.
//            fprintf(filename,localpacket->filedata);
//            check_recv[localpacket->frag_no] = true;
//        }
        
//        strcpy(localpacket->filedata,"ACK");
//        localpacket->size = strlen("ACK");
//        
//        send_back_data = packtoString(localpacket);
//        
//        
//        if(sendto(sockfd, localpacket, sizeof(localpacket), 0, (struct sockaddr *) &their_addr, addr_len) == -1){
//            fprintf(stderr, "sendto error\n");
//            exit(1);
//        }
//        
//        if(localpacket->frag_no == localpacket->total_frag){
//            printf("File %s transfer completed",filename);
//        }
//        break;
//        
    }
    fclose(receive_file_name);
    free(localpacket);

    close(sockfd);
    

    return 0;
}