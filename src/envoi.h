#if ! defined ( ENVOI_H_ )
#define ENVOI_H_

#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "requete.h"

#define NOTHING_SENT -3
#define SENDING_HEADER -2
#define HEADER_SENT -1
#define SENDING_IMAGE 0
#define IMAGE_SENT 1
#define FRAGMENT_SENT 2

#define FAIL(x) if(x) {\
	perror(#x);}

#define FAIL_SEND(x) if(x == -1) {\
	perror(#x);}

#define FAIL_FATAL(x) if(x) {\
	perror(#x);exit(EXIT_FAILURE);}

struct envoi {
    int state;
    int clientSocket;
    
    char type; //TCP_PULL, TCP_PUSH, UDP_PULL ou UDP_PUSH

    int currentPos; //Position dans l'envoi
    int bufLen; //Longueur du buffer
    char* buffer; //Buffer courant
    
    int posDansImage; //Position du fragment dans l'image
    int tailleMaxFragment; //Taille max du fragment

    FILE* curFile;
    int fileSize;
};

double getTime();
double timeInterval(double t1, double t2);

void sendImage(struct videoClient* videoClient);

void createHeaderTCP(struct videoClient* videoClient);
void sendHeaderTCP(struct videoClient* videoClient);

void createImageTCP(struct videoClient* videoClient);
void sendImageTCP(struct videoClient* videoClient);

void createHeaderUDP(struct videoClient* videoClient);
void sendHeaderUDP(struct videoClient* videoClient);

void createFragment(struct videoClient* videoClient);
void sendFragment(struct videoClient* videoClient);


#endif // ENVOI_H_
