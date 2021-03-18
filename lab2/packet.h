#ifndef PACKET_H
#define PACKET_H

struct packet { 
	unsigned int total_frag;
 	unsigned int frag_no; 
	unsigned int size; 
	char* filename; 
	char filedata[1000]; 
};
char* packtoString(struct packet *pack);
struct packet* stringtoPack(char* str);
void printPack(struct packet* pack);
#endif
