#include "envoi.h"
#include <errno.h>

double getTime() {
    struct timeval timev;
    
    gettimeofday(&timev, NULL);
    double micro = (double)timev.tv_usec;
    double sec = (double)timev.tv_sec;
    
    return sec + micro/1000000;
}

void sendImage(struct videoClient* videoClient) {

    struct envoi* env = videoClient->envoi;

    if(env->type == ENVOI_TCP /*&& videoClient->etat == RUNNING*/) {
        if(env->state == NOTHING_SENT /*&& videoClient->dernierEnvoi*/ ) {
            createHeaderTCP(env);
        } else if(env->state == SENDING_HEADER) {
            sendHeaderTCP(env);
        } else if(env->state == HEADER_SENT) {
            createImageTCP(env);
        } else if(env->state != IMAGE_SENT) {
            sendImageTCP(env);
        }
    } else { //ENVOI_UDP
        //TODO: Faire l'envoi udp
    }
    
    
}

void createHeaderTCP(struct envoi* env) {
    env->buffer = malloc(128*sizeof(char));
    memset(env->buffer,'\0',128*sizeof(char));
    
    //Taille	
	//fseek plante je ne sait pas pourquoi
    fseek(env->curFile, 0, SEEK_END);
    env->fileSize = ftell(env->curFile);
    fseek(env->curFile, 0, SEEK_SET);
    char sBufferTaille[16];
    sprintf(env->buffer, "%d\r\n%d\r\n", env->id,env->fileSize);
    
    env->state = SENDING_HEADER;
    env->currentPos = 0;
    env->bufLen = strlen(env->buffer);
    
    puts("header cree");
    sendHeaderTCP(env);
}

void sendHeaderTCP(struct envoi* env) {
  
	int nbSent;	
   	do
	{
    nbSent = send(env->clientSocket, env->buffer, env->bufLen, MSG_NOSIGNAL);
    FAIL(nbSent); 
    env->currentPos += nbSent;
    printf("car envoyes (header) : %d_%d/%d\n", nbSent, env->currentPos, env->bufLen);
	
	char * tmp = (char *)malloc(128*sizeof(char));
	memset(tmp,'\0',128*sizeof(char));
	int i;
	for (i = env->currentPos+1; i < env->bufLen; i++)
	{
		tmp[i-env->currentPos+1] = env->buffer[i]; //on décale le buffer
	}
	
	char * tmp2;
	tmp2 = env->buffer;
	env->buffer = tmp;
	env->bufLen = strlen(env->buffer);
	free(tmp2);
	
	} while (errno == EAGAIN); //on boucle tant que le buffer n'est pas plein

    
    if(env->currentPos >= env->bufLen) {
        env->state = HEADER_SENT;
        free(env->buffer);
        puts("header sent");
        createImageTCP(env);
    }
    
    
}

void createImageTCP(struct envoi* env) {
    env->buffer = malloc(env->fileSize*sizeof(char));
	memset(env->buffer,'\0',env->fileSize*sizeof(char));
    env->currentPos = 0;
    env->bufLen = env->fileSize;
    
    int retour = fread(env->buffer, sizeof(char), env->fileSize, env->curFile);
    if(retour == -1) {
        perror("fread raté");
    }
    env->state = SENDING_IMAGE;
    puts("image cree");
    sendImageTCP(env);
}

void sendImageTCP(struct envoi* env) {

	int nbSent;
	do
	{
    nbSent = send(env->clientSocket, env->buffer, env->bufLen, MSG_NOSIGNAL);
    FAIL(nbSent);
    env->currentPos += nbSent;
    printf("car envoyes (image) : %d_%d/%d\n", nbSent, env->currentPos, env->bufLen);
	
	char * tmp = (char *)malloc(env->fileSize*sizeof(char));
	memset(tmp,'\0',env->fileSize*sizeof(char));
	int i;
	for (i = env->currentPos+1; i < env->bufLen; i++)
	{
		tmp[i-env->currentPos+1]=env->buffer[i];
	}
	
	char * tmp2;
	tmp2 = env->buffer;
	env->buffer = tmp;
	env->bufLen = strlen(env->buffer);
	free(tmp2);
	} while (errno == EAGAIN);

    
    printf("car envoyes (image) : %d_%d/%d\n", nbSent, env->currentPos, env->bufLen);
    if(env->currentPos >= env->bufLen) {
        env->state = IMAGE_SENT; //Pour le moment on envoie toujours la même image
        //env->state = NOTHING_SENT;
		free(env->buffer);
        close(env->curFile);
		free(env);
        puts("Image envoyée");
    } else {
        //send(env); //TODO: supprimer après les tests
    }
}
