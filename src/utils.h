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
#include <fcntl.h> // Pour déclarer non bloquant
#include <sys/utsname.h> //Pour connaitre version noyau

#include "requete.h"
#include "cata.h"

#define MAX_EVENTS 10
#define BASE_CLIENTS 32
#define BASE_FICHIERS 32 

struct sockClient {
	int sock;
	int isGET;
	struct requete requete;
	struct videoClient videoClient;
};
struct tabClients {
	int nbClients;
	struct sockClient * clients;
};
struct infosVideo {
	char type;
	double fps;
	int nbImages;
	char ** images;
};
struct tabFichiers {
	int nbFichiers;
	int * socks;
	struct infosVideo * infosVideos;
};

void send_get_answer(int fd, char * catalogue);

int createSockEvent(int epollfd, int port);

int createSockClientEvent(int epollfd, int sock);

void createFichier(int epollfd, struct tabFichiers * tabFichiers, int port, int * baseFichierCourante);

void addImage(char * uneImage, struct infosVideo * infos);

void connectClient(int epollfd, struct tabClients * tabClients, struct tabFichiers * tabFichiers, int sock , int * baseCourante, int isGet);

int connectDataTCP(int epollfd, int sock, int port, int type);
#endif // UTILS_H_
