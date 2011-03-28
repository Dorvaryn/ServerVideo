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
#define SENDING_FRAGMENT 2


struct envoi {
    int state;
    
    int bufLen; //Longueur du buffer
    char* originBuffer;
	char* buffer; //Buffer courant

	int posDansImage;
	int tailleMaxFragment;
	int tailleFragment;
	char more;
	struct sockaddr * dest_addr;

    FILE* curFile;
    int fileSize;
};

double getTime();

double timeInterval(double t1, double t2);

void sendImage(struct videoClient* videoClient);

void createHeaderTCP(struct videoClient* videoClient);

void createImageTCP(struct videoClient* videoClient);

void sendTCP(struct videoClient* videoClient);

void createHeaderUDP(struct videoClient* videoClient);

void createFragment(struct videoClient* videoClient);

void sendUDP(struct videoClient* videoClient);



#endif // ENVOI_H_
