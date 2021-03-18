#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "packet.h"

#define MAX_NAME 10
#define MAX_PASS 15
#define USER_NUM 3
#define MAX_ID 10
#define MAX_LENGTH 2000
#define MAX_USER 10
#define MAX_SESSION 10

struct user{
	char name[MAX_NAME];
	char pass[MAX_PASS];
	int active;
	int sockfd;
	struct session* cur_ses;
	
};

struct session{
	char session_ID[MAX_ID];
	int user_num;
	struct user* users[MAX_USER];
};

struct user users[USER_NUM]={
{.name="XU", .pass="12345", .active=0, .sockfd=-1,.cur_ses=NULL}, 
{.name="WU", .pass="12345", .active=0, .sockfd=-1,.cur_ses=NULL},
{.name="SHAN", .pass="12345", .active=0, .sockfd=-1,.cur_ses=NULL}
};

struct user* find_user(const char* user_name){
	for(int i=0; i<USER_NUM;i++){
		if(strcmp(users[i].name, user_name)==0)
			return &users[i];
	}
	return NULL;
}


struct session* sessions[MAX_SESSION];


struct session* find_session(const char* session_id) {
  for (int i = 0; i < MAX_SESSION; i++) {
    if (sessions[i] != NULL &&
        (strcmp(sessions[i]->session_ID, session_id) == 0)) {
      return sessions[i];
    }
  }
  return NULL;
}


int reply(int sockfd, unsigned int type, char* data ){

	struct  packet p;
	p.type=type;
	p.size=strlen(data);
	strcpy(p.source, "server");
	strcpy(p.data, data);
	char* str=packtoString(&p);
        printf("[Server] reply: %s\n",str);
	int sen=send(sockfd, str, strlen(str)+1, 0);
	if(sen==-1){
		perror("reply failure");
		return -1;	
	}
	return 0;
}


int session_mess(int sockfd, unsigned int type, char* source,  char* data ){

	struct  packet p;
	p.type=type;
	p.size=strlen(data);
	strcpy(p.source, source);
	strcpy(p.data, data);
	char* str=packtoString(&p);
	int sen=send(sockfd, str, strlen(str)+1, 0);
	if(sen==-1){
		perror("session mess failure\n");
		return -1;	
	}
	return 0;
}

int auth(struct packet* p,int sockfd){
    
	if(p->type==LOGIN){
		for(int i=0; i<USER_NUM; i++){
			if ((strcmp(users[i].name, p->source) == 0) 
				&& (strcmp(users[i].pass, p->data) == 0)) {
        			if (users[i].active) {
          				reply(sockfd, LO_NCK, "user already logged in");
          				return 0;
        			} 
				else {        	
					reply(sockfd, LO_ACK, "success");		
          				users[i].active = 1;
          				users[i].sockfd = sockfd;
          				printf("User %s connected\n", p->source);
          				return 1;
        			}
      			}
    		}
		reply(sockfd, LO_NCK, "Wrong name or password\n");
		return 0;	
	}	
	else {
		printf("message type error (login)\n");
		reply(sockfd, LO_NCK, "login type error\n");
		return 0;
	}
	return 0;
}

void logout(char* user_name,fd_set* fds , int rep);
void join_session(char*user_name, char* session_ID);
void leave_session(char* user_name, int Rep);
void message(char*user_name, char* session_ID);
void list();
void create_session(char*user_name, char* session_ID);




int main(int argc, char const * argv[]) {
	for(int i=0; i<MAX_SESSION;i++){
		sessions[i]=NULL;
	}	

	if (argc != 2) {
		printf("Error: wrong arg number");
		exit(1);
  	}
	unsigned int port = atoi(argv[1]);

	/*get all available addrinfo with spec port# for creating server*/
	struct addrinfo hints, *res;
  	bzero(&hints, sizeof(hints));
  	hints.ai_flags = AI_PASSIVE; // my IP
  	hints.ai_family = AF_INET; // IPv4
  	hints.ai_socktype = SOCK_STREAM; // TCP
	int getaddr = getaddrinfo(NULL, argv[1], &hints, &res);
	if(getaddr==-1) {
		perror("getaddr\n");
		exit(1);
	}

/*opening server*/
	int sockfd;
	struct addrinfo* p;
	for(p=res;p!=NULL;p=p->ai_next ){		
		sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	
		if (sockfd == -1) {
      			continue;
    		}
		int err = bind(sockfd, p->ai_addr, p->ai_addrlen);
    		if (err == -1) {
      			close(sockfd);		
      			continue;
    		}
		break;
	}		
	if(p==NULL) {printf("No available addr.\n"); exit(1);}
	freeaddrinfo(res);

/*start to listen*/	
	int backlog=25;
	if(listen(sockfd, backlog)!=0){
		close(sockfd);
		perror("fail on listen\n");
		exit(1);
	}
	printf("Ready for connections\n");

/*get all requests*/
	fd_set fds;
	int max_sockfd=sockfd;
	while(1){
		FD_ZERO(&fds);
		FD_SET(sockfd, &fds);
		for(int i=0; i<USER_NUM;i++){
			if(users[i].active&&users[i].sockfd!=-1){
				FD_SET(users[i].sockfd,&fds);
				printf("set %d\n", i);
			}
			if(users[i].sockfd>max_sockfd) max_sockfd=users[i].sockfd;
		}
		//printf("select...\n");
		int sel=select(max_sockfd+1, &fds, NULL,NULL,NULL);
		if(sel==-1)perror("select\n"); 
		

/*------------------------------------------------------------------------------------*/

		if(FD_ISSET(sockfd,&fds)){  //sockfd readable new connection
		
			struct sockaddr new_addr;
			socklen_t new_addrlen;
			int new_fd=accept(sockfd, &new_addr, &new_addrlen);
			if(new_fd<0){
				perror("accept\n");
				continue;			
			}
			char buf[MAX_LENGTH];
			int rec=recv(new_fd, buf,MAX_LENGTH, 0);
			if(rec==-1){ perror("recv from\n"); close(new_fd);}			
 			struct packet* p=stringtoPack(buf);
			if(!auth(p, new_fd))
				close(new_fd);			
		}
		
		else{  //request with existing users
			for(int i=0; i<USER_NUM;i++){
				if(users[i].active&&users[i].sockfd>0){
					if(FD_ISSET(users[i].sockfd, &fds)){
						char buf[MAX_LENGTH];
						
						int rec=recv(users[i].sockfd, buf,MAX_LENGTH, 0);
						if(rec==-1){ 
							perror("recv from\n"); 
							continue;
						}		
						if(rec==0){
							logout( users[i].name,&fds, 0);
               				perror("client: not connect");
                			continue;

						}	
 						struct packet* p=stringtoPack(buf);
						
						switch(p->type){
							case LOGOUT:
								logout(p->source, &fds, 1);
								break;
							case JOIN:
                                                            printf(" time is JOIN!!!\n");
								join_session(p->source, p->data);
								break;	
							case LEAVE_SESSION:
								leave_session(p->source,1);
								break;
							case CREATE_SESSION:
								create_session(p->source,p->data);
								break;
							case MESSAGE:
								message(p->source, p->data);
								break;
							case QUERY:
								list(p->source);
								break;
						}			
					}
				}
			}
		}

	



	}
	

}

void logout(char* user_name,fd_set* fds , int rep){
	for(int i=0; i<USER_NUM;i++){
		if(strcmp(users[i].name,user_name)==0){
			users[i].active=0;
			leave_session(user_name,0);
			users[i].cur_ses=NULL;			
			if(rep) reply(users[i].sockfd, LOGOUT_ACK, "logout success\n");
			close(users[i].sockfd);
			users[i].sockfd=-1;
			FD_CLR(users[i].sockfd, fds);
			printf("logout success.%s\n",users[i].name);			
			return;
		}	
	}
	//reply(users[i].sockfd, LOGOUT_NCK, "logout unsuccess\n");
	printf("user name not exist: %s\n", user_name);
	return; 
}

void join_session(char*user_name, char* session_ID){
	struct user* u=find_user(user_name);
	if(u==NULL){printf("[JOIN]no such user.\n"); return;}
	struct session* s=find_session(session_ID);	
	if(s==NULL){printf("[JOIN]no such session.\n");
		 reply(u->sockfd, JN_NCK, "[JOIN]unsuccess:no such session\n");return;} 
	
	if(s->user_num>=MAX_USER){ printf("[JOIN]session full, stay in previous.\n"); 
 	reply(u->sockfd, JN_NCK, "[JOIN] session full, stay in previous.\n");return;}
	leave_session(user_name,0);
	u->cur_ses=s;	
	s->user_num++;
	for(int i=0; i<MAX_USER;i++){
		if(s->users[i]==NULL){
			s->users[i]=u;	
			break;	
		}
	}
	printf("[JOIN] %s join session %s.\n", user_name, session_ID);
	reply(u->sockfd, JN_ACK, "[JOIN] join success\n");
	return; 
}

void create_session(char*user_name, char* session_ID){
	
	struct user* u=find_user(user_name);
	if(u==NULL){printf("[CREATE]no such user.\n"); return;}
	struct session* s=find_session(session_ID);
	if(s!=NULL){printf("[CREATE]session exist.\n"); 
		reply(u->sockfd, CREATE_NACK, "[CREATE] unsuccess\n");return;} 
	leave_session(user_name,0);
	for(int i=0; i<MAX_SESSION;i++){
		if(sessions[i]==NULL){
			sessions[i]=malloc(sizeof(struct session));
			sessions[i]->user_num=1;
			sessions[i]->users[0]=u;
			for(int j=1; j<MAX_USER;j++) sessions[i]->users[j]=NULL;
			strcpy(sessions[i]->session_ID, session_ID);
			u->cur_ses=sessions[i];
			if(sessions[i]!=NULL)printf("new!\n");
			break;	
		}		
	}
	printf("[CREATE] %s create session %s.\n", user_name, session_ID);
	reply(u->sockfd, CREATE_ACK, "[CREATE]success\n");
	return;

}

void leave_session(char* user_name, int Rep){
	
	struct user* u=find_user(user_name);
	if(u==NULL){printf("[LEAVE]no such user.\n"); return;}
	if(u->cur_ses==NULL) {printf("[LEAVE]not in any session.\n");if(Rep) reply(u->sockfd, LEAVE_NCK, "[LEAVE] unsuccess\n"); return;}
	
	struct session* s=u->cur_ses;	
	u->cur_ses=NULL;
	s->user_num--;
	for(int i=0; i<MAX_USER;i++){
		
		if(s->users[i]!=NULL&&strcmp(user_name, s->users[i]->name)==0){
			s->users[i]=NULL;
			break;
		}
	}
	
	if(s->user_num==0){
		for(int i=0; i<MAX_SESSION;i++){
			if(sessions[i]==s) {
				sessions[i]=NULL;
				break;
			}
		}		
		free(s);

	}
	if(Rep) reply(u->sockfd, LEAVE_ACK, "[LEAVE] success\n");
}

void message(char*user_name, char* data){
	struct user* u=find_user(user_name);
	if(u==NULL){printf("[MESS]no such user.\n"); return;}
	if(u->cur_ses==NULL){
		printf("[MESS]user not in any session.\n"); return;
	}
        printf("this is content: %d\n",u->cur_ses->user_num);
	for (int i = 0; i < MAX_USER; i++) {
    		if (u->cur_ses->users[i] != NULL) {
                    printf("username is: %s\n",u->cur_ses->users[i]->name);
      			session_mess(u->cur_ses->users[i]->sockfd, MESSAGE, user_name, data);
    		}
  	}
	printf("[MESS]message sent.\n");
	return;
}

int get_session_info(struct session* s, char* dest) {
  
	//printf("in get session.\n");
  if (dest == NULL) return 1;
  snprintf(dest, MAX_DATA, "\n ID [%s]\n Capacity [%d/%d]\n ", s->session_ID, s->user_num, MAX_USER);
  return 0;
}

void list(char* user_name){
	struct user* u=find_user(user_name);
	if(u==NULL){printf("[LIST]no such user.\n"); return;}	
	size_t cur_pos = 0;
  	char buf[MAX_LENGTH];
	char dest[MAX_LENGTH];
		
  	for (int i = 0; i < MAX_SESSION; i++) {
    	if (sessions[i] != NULL) {
		printf("session not null.\n");
     		get_session_info(sessions[i], buf);
		printf("buf: %s\n", buf);
    		strncpy(dest + cur_pos, buf, strlen(buf));
     		cur_pos += strlen(buf);
    	}
  	}
	
	for(int i=0; i<USER_NUM;i++){
		if(users[i].active==1){
			strncpy(dest + cur_pos, users[i].name, strlen(buf));
			cur_pos += strlen(users[i].name);
			strncpy(dest + cur_pos," ", 1);
			cur_pos++;		
		}
	}
	
        printf("dest is: %s\n",dest);
	reply(u->sockfd, QUERY_ACK, dest);
        
	return;

}

