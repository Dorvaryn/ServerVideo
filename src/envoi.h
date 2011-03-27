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

#define ENVOI_TCP 0
#define ENVOI_UDP 1

#define FAIL(x) if(x) {\
	perror(#x);}

#define FAIL_SEND(x) if(x == -1) {\
	perror(#x);}

#define FAIL_FATAL(x) if(x) {\
	perror(#x);exit(EXIT_FAILURE);}

struct envoi {
    int state;
    
    int bufLen; //Longueur du buffer
    char* originBuffer;
	char* buffer; //Buffer courant

    FILE* curFile;
    int fileSize;
};

double getTime();

void sendImage(struct videoClient* videoClient);

void createHeaderTCP(struct videoClient* videoClient);

void createImageTCP(struct videoClient* videoClient);

void sendTCP(struct videoClient* videoClient);


#endif // ENVOI_H_
