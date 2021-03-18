/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
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
#include <pthread.h>
#include <time.h>

#define MAX_NAME 50
#define MAX_DATA 1000



char buffer[1000];

char *CMD_LOGIN = "/login";
char *CMD_LOGOUT = "/logout\n";
char *CMD_JOINSESSION = "/joinsession";
char *CMD_LEAVESESSION = "/leavesession";
char *CMD_CREATESESSION = "/createsession";
char *CMD_LIST = "/list\n";
char *CMD_QUIT = "/quit\n";
char *CMD_INVITE = "/invite";
int NOT_CONNECT_TO_SERVER = -1;
bool joined_session = false;
int totalsession;

clock_t begin, end;
double time_spent;

enum {
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
    QUERY_NCK
};

struct packet {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

void* count_time(void * ptr) {
    while (time_spent < 5.0) {
        end = clock();
        time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
        //printf(" %lf\n", time_spent);

    }
}

char* packtoString(struct packet *pack) {
    int size = sizeof (unsigned)*2 + sizeof (pack->source) + sizeof (pack->data) + 3 * sizeof (':') + 1;
    char* string = (char*) malloc(size);
    int loc = 0;
    sprintf(string + loc, "%u:", pack->type);
    loc = strlen(string);
    sprintf(string + loc, "%u:", pack->size);
    loc = strlen(string);
    sprintf(string + loc, "%s:", pack->source);
    loc = strlen(string);
    memcpy(string + loc, pack->data, pack->size);
    return string;

}

struct packet* stringtoPack(char* str) {
    struct packet *a = malloc(sizeof (struct packet));
    char* ptr;
    a->type = (unsigned int) strtoul(str, &ptr, 10);
    a->size = (unsigned int) strtoul(ptr + sizeof (char), &ptr, 10);

    char* e = strchr(ptr + 1, ':');
    memcpy(a->source, ptr + 1, e - ptr - 1);
    a->source[e - ptr - 1] = '\0';
    ptr = e;
    memcpy(a->data, ptr + 1, a->size);
    //a->data[a->size]='\0';

    return a;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

void login(int *sockfd, char *client_ID) {
    char *password, *server_IP, *server_port, *temp_ID;
    //temp_ID = strtok(NULL, " ");
    password = strtok(NULL, " ");
    server_IP = strtok(NULL, " ");
    server_port = strtok(NULL, " \n");

    if (!client_ID || !password || !server_IP || !server_port) {
        printf("error input format.\n");
        return;
    } else if (*sockfd != NOT_CONNECT_TO_SERVER) {
        printf("error command, can only login in to one server at same time!\n");
        return;
    } else {
        int numbytes;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        char s[INET6_ADDRSTRLEN];

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        //printf("i am here ``````");
        if ((rv = getaddrinfo(server_IP, server_port, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return;
        }

        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((*sockfd = socket(p->ai_family, p->ai_socktype,
                    p->ai_protocol)) == -1) {
                perror("client: socket");
                continue;
            }
            if (connect(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(*sockfd);
                perror("client: connect");
                continue;
            }
            break;
        }

        if (p == NULL) {
            *sockfd = NOT_CONNECT_TO_SERVER;
            fprintf(stderr, "client: failed to connect\n");
            return;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr), s, sizeof s);
        printf("client: connecting to %s\n", s);
        freeaddrinfo(servinfo);

        struct packet packet;
        packet.type = LOGIN;
        strncpy(packet.source, client_ID, MAX_NAME);
        strncpy(packet.data, password, MAX_DATA);
        packet.size = strlen(packet.data);

        char *data;
        data = packtoString(&packet);

        if ((numbytes = send(*sockfd, data, strlen(data), 0)) == -1) {
            fprintf(stderr, "client: send error\n");
            close(*sockfd);
            *sockfd = NOT_CONNECT_TO_SERVER;
            return;
        }

        if ((numbytes = recv(*sockfd, buffer, 1000, 0)) == -1) {
            fprintf(stderr, "client: recv error\n");
            close(*sockfd);
            *sockfd = NOT_CONNECT_TO_SERVER;
            return;
        }
        //printf("this is login recv buffer %s\n\n",buffer);

        struct packet* reveicedPacket = (struct packet*) malloc(sizeof (struct packet));
        reveicedPacket = stringtoPack(buffer);

        if (reveicedPacket->type == LO_ACK) {
            printf("login successful. \n");
            return;
            //        } else if (reveicedPacket->type == LO_NCK) {
            //            printf("login failed.\n");
            //            close(*sockfd);
            //            *sockfd = NOT_CONNECT_TO_SERVER;
            //            return;
        } else {
            printf("error packet received: type %d .\n", reveicedPacket->type);
            close(*sockfd);
            *sockfd = NOT_CONNECT_TO_SERVER;
            return;
        }

    }
}

void logout(int *sockfd, char *client_ID) {
    int numbytes;
    char *data;
    struct packet packet;
    packet.type = LOGOUT;
    strncpy(packet.source, client_ID, MAX_NAME);
    packet.size = 0;
    data = packtoString(&packet);

    if (*sockfd == NOT_CONNECT_TO_SERVER) {
        printf("logout error, user not login!\n");
        return;
    }

    if ((numbytes = send(*sockfd, data, strlen(data), 0)) == -1) {
        fprintf(stderr, "client: send\n");
        return;
    }

    if ((numbytes = recv(*sockfd, data, 1000, 0)) == -1) {
        fprintf(stderr, "client join_session: recv error\n");
        return;
    }

    struct packet* reveicedPacket = (struct packet*) malloc(sizeof (struct packet));
    reveicedPacket = stringtoPack(data);

    if (reveicedPacket->type == LOGOUT_ACK) {

        joined_session = false;
        close(*sockfd);
        *sockfd = NOT_CONNECT_TO_SERVER;
        printf("logout successful.\n");
        return;
    } else if (reveicedPacket->type == LOGOUT_NCK) {
        printf("logout failed.\n");
        return;
    } else {
        printf("error packet received: type %d.\n", reveicedPacket->type);
        return;
    }



}

void joinsession(int *sockfd, char *client_ID) {
    char *session_ID;

    session_ID = strtok(NULL, " \n");
    if (session_ID == NULL) {
        printf("not enough args.\n");
        return;
    }

    //    if (joined_session) {
    //        printf("you have joined a session before!\n");
    //        return;
    //    }


    if (session_ID == NULL) {
        printf("error input format!\n");
        return;
    } 
    
    else if (*sockfd == NOT_CONNECT_TO_SERVER) {
        printf("error, user not login!\n");
        return;
    } 
    
    else {

        int numbytes;
        char *data;
        struct packet packet;
        packet.type = JOIN;
        strncpy(packet.source, client_ID, MAX_NAME);
        printf("join client ID is: %s\n", client_ID);
        strncpy(packet.data, session_ID, MAX_DATA);
        packet.size = strlen(packet.data);
        data = packtoString(&packet);
        //printf("join data is: %s",data);


        if ((numbytes = send(*sockfd, data, strlen(data), 0)) == -1) {
            fprintf(stderr, "client join session: send error\n");
            return;
        }

        if ((numbytes = recv(*sockfd, buffer, 1000, 0)) == -1) {
            fprintf(stderr, "client join_session: recv error\n");
            return;
        }
        //printf("\nthis is join session recv buffer %s\n",buffer);

        struct packet* reveicedPacket = (struct packet*) malloc(sizeof (struct packet));
        reveicedPacket = stringtoPack(buffer);

        if (reveicedPacket->type == JN_ACK) {
            joined_session = true;
            totalsession++;
            printf("join session successful.\n");
            printf("you have joined %d session in total.\n", totalsession);
            return;
        } 
        
        else if (reveicedPacket->type == JN_NCK) {
            printf("join session failed.\n");
            printf("	%s", reveicedPacket->data);
            return;
        } 
        
        else {
            printf("error packet received: type %d. \n", reveicedPacket->type);
            return;
        }

    }
}

void leavesession(int *sockfd, char *client_ID) {
    int numbytes;
    char *data;
    struct packet packet;
    packet.type = LEAVE_SESSION;
    strncpy(packet.source, client_ID, MAX_NAME);
    packet.size = 0;
    data = packtoString(&packet);

    if (*sockfd == NOT_CONNECT_TO_SERVER) {
        printf("leave error, user not login!\n");
        return;
    }
    
    if (!joined_session) {
        printf("leave error, user has not join any session!\n");
        return;
    }

    if ((numbytes = send(*sockfd, data, strlen(data), 0)) == -1) {
        fprintf(stderr, "client leave session: send\n");
        return;
    }

    if ((numbytes = recv(*sockfd, buffer, 1000, 0)) == -1) {
        fprintf(stderr, "client leave session: recv error\n");
        return;
    }
    //printf("this is leave session buffer recv: %s\n",buffer);

    struct packet* reveicedPacket = (struct packet*) malloc(sizeof (struct packet));
    reveicedPacket = stringtoPack(buffer);

    if (reveicedPacket->type == LEAVE_ACK) {
        totalsession--;
        
        if(totalsession == 0) joined_session = false;
        
        printf("leave session successful.\n");
        return;
    } 
    
    else if (reveicedPacket->type == LEAVE_NCK) {
        printf("leave session failed.\n");
        return;       
    } 
    
    else {
        printf("error packet received: type %d.\n", reveicedPacket->type);
        return;
    }
}

void createsession(int *sockfd, char *client_ID) {
    char *session_ID;
    session_ID = strtok(NULL, " \n");

    if (!session_ID) {
        printf("create error input format!\n");
        return;
    } else if (*sockfd == NOT_CONNECT_TO_SERVER) {
        printf("create error, user not login!\n");
        return;
    } else {
        int numbytes;
        char *data;
        struct packet packet;
        packet.type = CREATE_SESSION;
        strncpy(packet.source, client_ID, MAX_NAME);
        strncpy(packet.data, session_ID, MAX_DATA);
        packet.size = strlen(packet.data);
        data = packtoString(&packet);

        if ((numbytes = send(*sockfd, data, strlen(data), 0)) == -1) {
            fprintf(stderr, "client create session: send error\n");
            return;
        }

        if ((numbytes = recv(*sockfd, buffer, 1000, 0)) == -1) {
            fprintf(stderr, "client create session: recv error\n");
            return;
        }
        //printf("this is create session buffer recv: %s\n",buffer);

        struct packet* reveicedPacket = (struct packet*) malloc(sizeof (struct packet));
        reveicedPacket = stringtoPack(buffer);

        if (reveicedPacket->type == CREATE_ACK) {
            joined_session = true;
            printf("create session successful.\n");
            return;
        } else if (reveicedPacket->type == CREATE_NACK) {
            printf("create session failed.\n");
            return;
        } else {
            printf("error packet received: type %d.\n", reveicedPacket->type);
            return;
        }
    }
}

void list(int *sockfd, char *client_ID) {
    if (*sockfd == NOT_CONNECT_TO_SERVER) {
        printf("list error, user not login!\n");
        return;
    } else {
        int numbytes;
        char *data;
        struct packet packet;
        packet.type = QUERY;
        strncpy(packet.source, client_ID, MAX_NAME);
        printf("list client ID: %s\n", client_ID);
        packet.size = 0;
        data = packtoString(&packet);
        //printf("list send data: %s\n",data);

        if ((numbytes = send(*sockfd, data, strlen(data), 0)) == -1) {
            fprintf(stderr, "client list: send error\n");
            return;
        }

        if ((numbytes = recv(*sockfd, buffer, 1000, 0)) == -1) {
            fprintf(stderr, "client list: recv error\n");
            return;
        }
        printf("this is list buffer recv: %s\n", buffer);

        struct packet* reveicedPacket = (struct packet*) malloc(sizeof (struct packet));
        reveicedPacket = stringtoPack(buffer);
        //printf("buf%s.\n",buffer);
        if (reveicedPacket->type == QUERY_ACK) {
            char str[1000];
            strcpy(str, reveicedPacket->data);
            printf("show list successful.\n");
            printf("%s", str);
            return;
        } else if (reveicedPacket->type == QUERY_NCK) {
            printf("show list failed.\n");
            return;
        } else {
            printf("error packet received: type %d.\n", reveicedPacket->type);
            return;
        }
    }

}

void quit(int *sockfd, char *client_ID) {
    if (*sockfd == NOT_CONNECT_TO_SERVER) return;
    logout(sockfd, client_ID);
}

void send_message(int *sockfd, char *client_ID) {
    if (*sockfd == NOT_CONNECT_TO_SERVER) {
        printf("message error, user not login!\n");
        return;
    } 
    
    else if (!joined_session) {
        printf("message error, user has not join any session!\n");
        return;
    } 
    
    else {
        int numbytes;
        char *data;
        struct packet packet;
        packet.type = MESSAGE;
        strncpy(packet.source, client_ID, MAX_NAME);
        strncpy(packet.data, buffer, MAX_DATA);

        packet.size = strlen(packet.data);
        data = packtoString(&packet);
        //printf("send message is %s\n",data);

        if ((numbytes = send(*sockfd, data, strlen(data), 0)) == -1) {
            fprintf(stderr, "client send_message: send error\n");
            return;
        }

        if ((numbytes = recv(*sockfd, buffer, 1000, 0)) == -1) {
            fprintf(stderr, "client send_message: recv error\n");
            return;
        }
        //printf("this is message recv: %s\n",buffer);

        struct packet* reveicedPacket = (struct packet*) malloc(sizeof (struct packet));
        reveicedPacket = stringtoPack(buffer);

        if (reveicedPacket->type == MESSAGE) {
            printf("%s: %s\n", reveicedPacket->source, reveicedPacket->data);
            return;
        }
    }
}

int main() {
    char *command;
    char *client_ID, *temp_ID;
    char cpy[1000];
    int sockfd = NOT_CONNECT_TO_SERVER;
    fd_set fds;
    struct timeval tv;

    while (true) {

        FD_ZERO(&fds);
        FD_SET(fileno(stdin), &fds);
        //FD_SET(sockfd, &fds);
        tv.tv_sec = 60;

        if (sockfd > 0) {        
		FD_SET(sockfd, &fds); 
		select(sockfd + 1, &fds, NULL, NULL, NULL);
           /*int ret = select(fileno(stdin) + 1, &fds, NULL, NULL, &tv);
            if (ret == 0) {
                printf("Timeout! You will be logout out!!!.\n");
                logout(&sockfd, client_ID);
                continue;
            }*/
       	
        }
        
        else {
            select(fileno(stdin) + 1, &fds, NULL, NULL, NULL);
        }

        if (joined_session && FD_ISSET(sockfd, &fds)) {
            recv(sockfd, buffer, 1000, 0);
            struct packet* pack = stringtoPack(buffer);
            printf("%s: %s\n", pack->source, pack->data);
        } 
        
        /*else*/ if (FD_ISSET(fileno(stdin), &fds)) {
            fgets(buffer, sizeof (buffer), stdin);
            strcpy(cpy, buffer);

            if ((strlen(cpy) == 1)) continue;
            command = strtok(cpy, " ");

            if (strcmp(command, CMD_LOGIN) == 0) {
                //printf("i am in login!\n");
                temp_ID = strtok(NULL, " ");
                client_ID = malloc(strlen(temp_ID));
                strcpy(client_ID, temp_ID);
                //printf("ID is %s",client_ID);
                login(&sockfd, client_ID);
            } 
            
            else if (strcmp(command, CMD_LOGOUT) == 0) {
                //printf("i am in logout!\n");
                logout(&sockfd, client_ID);
            } 
            
            else if (strcmp(command, CMD_JOINSESSION) == 0) {
                //printf("i am in join!\n");
                joinsession(&sockfd, client_ID);
            } 
            
            else if (strcmp(command, CMD_LEAVESESSION) == 0) {
                //printf("i am in leave!\n");
                leavesession(&sockfd, client_ID);
            } 
            
            else if (strcmp(command, CMD_CREATESESSION) == 0) {
                //printf("i am in create!\n");
                createsession(&sockfd, client_ID);
            } 
            
            else if (strcmp(command, CMD_LIST) == 0) {
                //printf("i am in list!\n");
                list(&sockfd, client_ID);
            } 
            
            else if (strcmp(command, CMD_QUIT) == 0) {
                //printf("here to quit!\n");
                quit(&sockfd, client_ID);
                break;
            } 
            
            else {
                //printf("this is send message.\n");
                send_message(&sockfd, client_ID);
            }
        }
    }
    return 0;
}
