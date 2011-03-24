#include "envoi.h"

double getTime() {
    struct timeval timev;
    
    gettimeofday(&timev, NULL);
    double micro = (double)timev.tv_usec;
    double sec = (double)timev.tv_sec;
    
    return sec + micro/1000000;
}

void sendImage(struct videoClient* videoClient) {

    struct envoi* env = videoClient->envoi;

    if(env->type == ENVOI_TCP && videoClient->etat == RUNNING) {
        if(env->state == NOTHING_SENT /*&& videoClient->dernierEnvoi*/ ) {
            createHeaderTCP(env);
        } else if(env->state == SENDING_HEADER) {
            sendHeaderTCP(env);
        } else if(env->state == HEADER_SENT) {
            createImageTCP(env);
        } else {
            sendImageTCP(env);
        }
    } else { //ENVOI_UDP
        //TODO: Faire l'envoi udp
    }
    
    
}

void createHeaderTCP(struct envoi* env) {
    env->buffer = malloc(128*sizeof(char));
    env->buffer[0] = '\0';
    
    //Image_id
    strcat(env->buffer, env->fileName/*env->ids[env->id]*/);
    
    strcat(env->buffer, "\r\n");
    
    //Taille
    fseek(env->curFile, 0, SEEK_END);
    env->fileSize = ftell(env->curFile);
    fseek(env->curFile, 0, SEEK_SET);
    char sBufferTaille[16];
    sprintf(sBufferTaille, "%d", env->fileSize);
    
    strcat(env->buffer, "\r\n");
    
    env->state = SENDING_HEADER;
    env->currentPos = 0;
    env->bufLen = strlen(env->buffer);
    
    sendHeaderTCP(env);
}

void sendHeaderTCP(struct envoi* env) {
    
    int nbSent = send(env->clientSocket, env->buffer, sizeof(env->buffer), MSG_NOSIGNAL);
    
    env->currentPos += nbSent;
    
    if(env->currentPos == env->bufLen) {
        env->state = HEADER_SENT;
        free(env->buffer);
    }
    //send(env); //TODO: supprimer après les tests
}

void createImageTCP(struct envoi* env) {
    env->buffer = malloc(env->fileSize*sizeof(char));
    env->currentPos = 0;
    env->bufLen = env->fileSize;
    
    int retour = fread(env->buffer, sizeof(char), env->fileSize, env->curFile);
    if(retour == -1) {
        perror("fread raté");
    }
    env->state = SENDING_IMAGE;
    //send(env); //TODO: supprimer après les tests
}

void sendImageTCP(struct envoi* env) {

    int nbSent = send(env->clientSocket, env->buffer, sizeof(env->buffer), MSG_NOSIGNAL);
    
    env->currentPos += nbSent;
    
    if(env->currentPos == env->bufLen) {
        env->state = IMAGE_SENT;
        free(env->buffer);
        close(env->curFile);
    } else {
        //send(env); //TODO: supprimer après les tests
    }
}
