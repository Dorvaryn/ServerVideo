#if ! defined ( UTILS_H_ )
#define UTILS_H_

#define _GNU_SOURCE
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

//Maximum d'un mot dans les requetes du client
#define MAX_TOCKEN 256

//Protocoles
#define TCP_PULL 0
#define TCP_PUSH 1
#define UDP_PULL 2
#define UDP_PUSH 3

//Type de requete
#define BAD_REQUEST (-2)
#define NON_DEFINI (-1)
#define GET 1
#define START 2
#define PAUSE 3
#define END 4
#define ALIVE 5

//Erreur de converion char->int
#define PARSE_ERROR -2

//Etats
#define RUNNING 0
#define PAUSED 1
#define OVER 2

#define MAX_EVENTS 10
#define BASE_CLIENTS 32
#define BASE_FICHIERS 32 
#define BASE_IMAGES 2048 

#define FAIL(x) if(x == -1) {\
	perror(#x);}

#define FAIL_FATAL(x) if(x == -1) {\
	perror(#x);exit(EXIT_FAILURE);}

struct infosVideo {
	char type;
	double fps;
	int nbImages;
	char ** images;
};

struct requete {
	int type;
    
    int isOver;
    
    int imgId;
    int listenPort;
    int fragmentSize;
    
    int inWord;
    int space;
    int crlfCounter;
    
    char* mot;
    int motPosition; //position dans le mot lu
    
    int reqPosition; //position du mot dans la requete
};
struct videoClient {
	
	int clientSocket;
    
    char etat; //RUNNING, PAUSED ou OVER
    
    int id; //Image courante
    double dernierEnvoi; //pour gérer les ips
    double lastAlive;
    
    struct sockaddr_in dest_addr;
    struct sockaddr_in orig_addr;
    
    struct envoi* envoi;
    struct infosVideo* infosVideo;
};
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
struct flux {
	int sock;
	int port;
	struct infosVideo infosVideo;
};
struct tabFlux {
	int nbFlux;
	struct flux * flux;
};

void initReq(struct requete* req);

void send_get_answer(int fd, char * catalogue);

int createSockEventTCP(int epollfd, int port);

int createSockEventUDP(int epollfd, int port);

int createSockClientEvent(int epollfd, int sock);

void createFichier(int epollfd, struct tabFlux * tabFlux, int port, int * baseFichierCourante, int type);

void addImage(char * uneImage, struct infosVideo * infos);

void connectClient(int epollfd, struct tabClients * tabClients, struct tabFlux * tabFlux, int sock , int * baseCourante, int isGet);

void createEventPush(int epoll, int csock);

void createEventPull(int epoll, int csock);

int connectDataTCP(int epollfd, int sock, int port, int type);
#endif // UTILS_H_
