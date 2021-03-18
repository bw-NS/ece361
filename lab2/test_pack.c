
#include "packet.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(){

	struct packet a;
	a.total_frag=1;	
 	a.frag_no=2; 
	a.size=3; 
	a.filename="123"; 
	a.filedata[0]='1';
	a.filedata[1]='4';
 	a.filedata[2]='5';
char* str=packtoString(&a);
	printf("%s\n",str);

char * string="99:98:5:foobor.txt:data\0";
struct packet* b=stringtoPack(string);
printPack(b);
}

