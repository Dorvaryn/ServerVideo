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

	if(videoClient->etat != OVER)
	{
		if(env->type == ENVOI_TCP /*&& videoClient->etat == RUNNING*/) {
			if(env->state == NOTHING_SENT /*&& videoClient->dernierEnvoi*/ ) {
				createHeaderTCP(env);
				sendTCP(env);
			}
			if(env->state == HEADER_SENT) {
				createImageTCP(env);
			}
			if(env->state != IMAGE_SENT) {
				sendTCP(env);
			}
		} else { //ENVOI_UDP
			//TODO: Faire l'envoi udp
		}
	}

}

void createHeaderTCP(struct envoi* env) {
	env->buffer = malloc(128*sizeof(char));
	env->originBuffer = env->buffer;
	memset(env->buffer,'\0',128*sizeof(char));

	//Taille	
	//fseek plante je ne sait pas pourquoi
	fseek(env->curFile, 0, SEEK_END);
	env->fileSize = ftell(env->curFile);
	fseek(env->curFile, 0, SEEK_SET);
	
	sprintf(env->buffer, "%d\r\n%d\r\n", env->id,env->fileSize);

	env->state = SENDING_HEADER;
	env->currentPos = 0;
	env->bufLen = strlen(env->buffer);

	puts("header cree");
}

void createImageTCP(struct envoi* env) {
	env->buffer = malloc(env->fileSize*sizeof(char));
	env->originBuffer = env->buffer;
	memset(env->buffer,'\0',env->fileSize*sizeof(char));
	env->currentPos = 0;
	env->bufLen = env->fileSize;

	int retour = fread(env->buffer, sizeof(char), env->fileSize, env->curFile);
	if(retour == -1) {
		perror("fread raté");
	}
	env->state = SENDING_IMAGE;
	puts("image cree");
}

void sendTCP(struct envoi* env) {

	int nbSent;
	do
	{	
		printf("socket du client : %d\n", env->clientSocket);
		
		if(env->clientSocket == 0)
		{ abort(); }
		nbSent = send(env->clientSocket, env->buffer, env->bufLen, MSG_NOSIGNAL);
		FAIL_SEND(nbSent);
		
		env->buffer += nbSent;
		env->bufLen -= nbSent;
		
		printf("car envoyes : %d/%d\n", nbSent, env->bufLen);


	} while (errno != EAGAIN && env->bufLen > 0);

	if(env->bufLen <= 0) {
		if(env->state == SENDING_HEADER)
		{
			env->state = HEADER_SENT;
			free(env->originBuffer);
			puts("Envoyé");
		}
		else
		{
			env->state = IMAGE_SENT; //Pour le moment on envoie toujours la même image
			fclose(env->curFile);
			free(env->originBuffer);
			puts("Envoyé");
		}
		//env->state = NOTHING_SENT;
	} else {
		//send(env); //TODO: supprimer après les tests
	}
}
