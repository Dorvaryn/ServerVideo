#if ! defined ( ENVOI_H_ )
#define ENVOI_H_

#include <string.h>
#include <stdlib.h>

#define NOTHING_SENT -3
#define SENDING_HEADER -2
#define HEADER_SENT -1
#define SENDING_IMAGE 0
#define IMAGE_SENT 1

struct envoiTcp {
    int state;
    int clientSocket;

    int currentPos; //Position dans l'envoi
    int bufLen; //Longueur du buffer
    char* buffer; //Buffer courant

    FILE* curFile;
    int fileSize;
};


void createHeaderTCP(struct envoiTcp* env);
void sendHeaderTCP(struct envoiTcp* env) ;

void createImageTCP(struct envoiTcp* env);
void sendImageTCP(struct envoiTcp* env);


#endif // ENVOI_H_
