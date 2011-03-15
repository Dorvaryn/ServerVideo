#if ! defined ( UTILS_H_ )
#define UTILS_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h> //Pour STDIN_FILENO

#include "requete.h"
#include "cata.h"

#define MAX_EVENTS 10
#define BASE_CLIENTS 32
#define BASE_FICHIERS 32 

struct sockClient {
	int sock;
	int isGET;
	struct requete requete;
};
struct tabClients {
	int nbClients;
	struct sockClient * clients;
};
struct tabFichiers {
	int nbFichiers;
	int * socks;
};

void send_get_answer(int fd, char * catalogue);

int createSockEvent(int epollfd, int port);

int createSockClientEvent(int epollfd, int sock);

void connectClient(int epollfd, struct tabClients * tabClients, int sock, int baseCourante, int isGet);

#endif // UTILS_H_
