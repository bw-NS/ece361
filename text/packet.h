/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   packet.h
 * Author: xuhong4
 *
 * Created on November 18, 2018, 1:07 PM
 */

#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include "packet.h"

#define MAX_NAME 50
#define MAX_DATA 1000

enum{
    LOGIN, 
    LO_ACK,
    LO_NCK,
    
    LOGOUT,
    LOGOUT_ACK,
    LOGOUT_NCK,
    
    JOIN,
    JN_ACK,
    JN_NCK,
    
    LEAVE_SESSION,
    LEAVE_ACK,
    LEAVE_NCK,
    
    CREATE_SESSION,
    CREATE_ACK,
    CREATE_NACK,
    
    MESSAGE,
    
    QUERY,
    QUERY_ACK,
    QUERY_NCK,

    BROADCAST,
    BROADCAST_ACK,
    BROADCAST_NCK,

    SWITCH,
    SWITCH_ACK,
    SWITCH_NCK
};
   
struct packet{
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

char* packtoString(struct packet *pack){
    	int size=sizeof(unsigned)*2+sizeof(pack->source)+sizeof(pack->data)+3*sizeof(':')+1;
	char* string=(char*)malloc(size);
	int loc=0;
	sprintf(string+loc,"%u:",pack->type);
	loc=strlen(string);
	sprintf(string+loc,"%u:",pack->size);
	loc=strlen(string);
	sprintf(string+loc,"%s:",pack->source);
	loc=strlen(string); 
	memcpy(string+loc, pack->data, pack->size);
	return string;
        
}

struct packet* stringtoPack(char* str){
    	struct packet *a= malloc(sizeof(struct packet));
	char* ptr;
	a->type= (unsigned int)strtoul(str, &ptr, 10);
	a->size=(unsigned int)strtoul(ptr+sizeof(char), &ptr, 10);

	char* e=strchr(ptr+1,':');
	memcpy(a->source,ptr+1, e-ptr-1);
	a->source[e-ptr-1]='\0';	
	ptr=e;
	memcpy(a->data,ptr+1, a->size);
	
	
	return a;
}

#endif
