#if ! defined ( ENVOI_H_ )
#define ENVOI_H_

#include <string.h>
#include <stdlib.h>

#define NOTHING_SENT -3
#define SENDING_HEADER -2
#define HEADER_SENT -1
#define SENDING_IMAGE 0
#define IMAGE_SENT 1

#define ENVOI_TCP 0
#define ENVOI_UDP 1

struct envoi {
    int state;
    int clientSocket;
    
    int type; //ENVOI_TCP ou ENVOI_UDP

    int currentPos; //Position dans l'envoi
    int bufLen; //Longueur du buffer
    char* buffer; //Buffer courant

    FILE* curFile;
    int fileSize;
};

void sendImage(struct envoi* env);

void createHeaderTCP(struct envoi* env);
void sendHeaderTCP(struct envoi* env) ;

void createImageTCP(struct envoi* env);
void sendImageTCP(struct envoi* env);


#endif // ENVOI_H_
