#include "packet.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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


