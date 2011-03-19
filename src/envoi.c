#include "envoi.h"

void send(struct envoiTcp* env) {
    if(env->state == NOTHING_SENT) {
        createHeaderTCP(env);
    } else if(env->state == SENDING_HEADER) {
        sendHeaderTCP(env);
    } else if(env->state == HEADER_SENT) {
        createImageTCP(env);
    } else {
        sendImageTCP(env);
    }
}

void createHeaderTCP(struct envoiTcp* env) {
    env->buffer = malloc(128*sizeof(char));
    env->buffer[0] = '\0';
    
    //Image_id
    strcat(env->buffer, env->ids[env->id]);
    
    strcat(env->buffer, "\r\n");
    
    //Taille
    fseek(fichier, 0, SEEK_END);
    env->fileSize = ftell(fichier);
    fseek(fichier, 0, SEEK_SET);
    char sBufferTaille[20] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    sprintf(sBufferTaille, "%d", env->fileSize);
    
    strcat(env->buffer, "\r\n");
    
    env->id = SENDING_HEADER;
    env->currentPos = 0;
    env->bufLen = strlen(env->buffer);
    
    sendHeaderTCP(env);
}

void sendHeaderTCP(struct envoiTcp* env) {
    
    int nbSent = send(env->clientSocket, env->buffer, sizeof(env->buffer), MSG_NOSIGNAL);
    
    env->currentPos += nbSent;
    
    if(env->currentPos == env->bufLen) {
        env->state = HEADER_SENT;
        free(env->buffer);
    }
    send(env); //TODO: supprimer après les tests
}

void createImageTCP(struct envoiTcp* env) {
    env->buffer = malloc(tailleFichier*sizeof(char));
    env->currentPos = 0;
    env->bufLen = env->fileSize;
    
    int retour = fread(buf, sizeof(char), tailleFichier, fichier);
    if(retour == -1) {
        perror("fread raté");
    }
    env->state = SENDING_IMAGE;
    send(env); //TODO: supprimer après les tests
}

void sendImageTCP(struct envoiTcp* env) {

    int nbSent = send(env->clientSocket, env->buffer, sizeof(env->buffer), MSG_NOSIGNAL);
    
    env->currentPos += nbSent;
    
    if(env->currentPos == env->bufLen) {
        env->state = IMAGE_SENT;
        free(env->buffer);
        close(curFile);
    } else {
        send(env); //TODO: supprimer après les tests
    }
}
