#include "envoi.h"
#include <errno.h>

double getTime() {
	struct timeval timev;

	gettimeofday(&timev, NULL);
	double micro = (double)timev.tv_usec;
	double sec = (double)timev.tv_sec;

	return sec + micro/1000000;
}

double timeInterval(double t1, double t2) {
    return (t1 < t2 ? t2-t1 : t2); //Evite le bug de minuit :)
}

void sendImage(struct videoClient* videoClient) {

	struct envoi* env = videoClient->envoi;

	if(videoClient->etat != OVER) //TODO: gérer la pause
	{
		if(env->type == TCP_PULL ) {
		
			if(env->state == NOTHING_SENT) {
				createHeaderTCP(videoClient);
			} else if(env->state == SENDING_HEADER) {
				sendHeaderTCP(videoClient);
			} else if(env->state == HEADER_SENT) {
				createImageTCP(videoClient);
			} else if(env->state != IMAGE_SENT) {
				sendImageTCP(videoClient);
			}
			
		} else if(env->type == TCP_PUSH) {
		
			if(env->state == NOTHING_SENT && timeInterval(videoClient->dernierEnvoi, getTime()) >= 1.0/videoClient->infosVideo->fps) {
				createHeaderTCP(videoClient);
				videoClient->dernierEnvoi = getTime();
			} else if(env->state == SENDING_HEADER) {
				sendHeaderTCP(videoClient);
			} else if(env->state == HEADER_SENT) {
				createImageTCP(videoClient);
			} else if(env->state != IMAGE_SENT) {
				sendImageTCP(videoClient);
			}
			
		} else if(env->type == UDP_PULL) {
		
			if(env->state == NOTHING_SENT) {
				createHeaderUDP(videoClient);
			} else if(env->state == SENDING_HEADER) {
				sendHeaderUDP(videoClient);
			} else if(env->state == HEADER_SENT) {
				createFragment(videoClient);
			} else if(env->state != FRAGMENT_SENT) {
				sendFragment(videoClient);
			}
			
		} else if(env->type == UDP_PUSH) {
			
			//TODO: push udp !
			
		}
	}

}

void createHeaderTCP(struct videoClient* videoClient) {
    struct envoi* env = videoClient->envoi;

	env->buffer = malloc(128*sizeof(char));
	memset(env->buffer,'\0',128*sizeof(char));

	//Taille
	fseek(env->curFile, 0, SEEK_END);
	env->fileSize = ftell(env->curFile);
	fseek(env->curFile, 0, SEEK_SET);

	sprintf(env->buffer, "%d\r\n%d\r\n", videoClient->id ,env->fileSize);

	env->state = SENDING_HEADER;
	env->currentPos = 0;
	env->bufLen = strlen(env->buffer);

	puts("header cree");
	sendHeaderTCP(videoClient);
}

void sendHeaderTCP(struct videoClient* videoClient) {
    struct envoi* env = videoClient->envoi;

	int nbSent;	
	do
	{
		nbSent = send(env->clientSocket, env->buffer, env->bufLen, MSG_NOSIGNAL);
		FAIL_SEND(nbSent); 
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

	} while (errno == EAGAIN && env->bufLen > 0); //on boucle tant que le buffer n'est pas plein


	if(env->currentPos >= env->bufLen) {
		env->state = HEADER_SENT;
		free(env->buffer);
		puts("header sent");
		createImageTCP(videoClient);
	}


}

void createImageTCP(struct videoClient* videoClient) {
    struct envoi* env = videoClient->envoi;

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
	sendImageTCP(videoClient);
}

void sendImageTCP(struct videoClient* videoClient) {
    struct envoi* env = videoClient->envoi;

	int nbSent;
	do
	{
		printf("socket du client : %d\n", env->clientSocket);
		nbSent = send(env->clientSocket, env->buffer, env->bufLen, MSG_NOSIGNAL);
		FAIL_SEND(nbSent);
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
	} while (errno == EAGAIN && env->bufLen > 0);


	printf("car envoyes (image) : %d_%d/%d\n", nbSent, env->currentPos, env->bufLen);
	if(env->currentPos >= env->bufLen) {
		close(env->curFile);
		puts("Image envoyée");
		if(env->type == TCP_PUSH) {
		    videoClient->id = (videoClient->id+1 < videoClient->infosVideo->nbImages ? videoClient->id+1 : 0);
		    videoClient->envoi->state = NOTHING_SENT;
		    videoClient->envoi->curFile = fopen(videoClient->infosVideo->images[videoClient->id], "r");
		} else {
		    env->state = IMAGE_SENT;
		    free(env->buffer);
		    free(videoClient);
		}
	}
}

//Cree un header pour un paquet
void createHeaderUDP(struct videoClient* videoClient) {
    //Prendre en compte la taille du fragment
    //(par rapport à la taille totale de l'image et du nbre de fragments envoyes)
    struct envoi* env = videoClient->envoi;
    
    env->buffer = malloc(128*sizeof(char));
	memset(env->buffer,'\0',128*sizeof(char));
	
	int tailleFragment;
	if(env->fileSize - env->posDansImage*env->tailleMaxFragment < env->tailleMaxFragment) {
	    tailleFragment = env->fileSize - env->posDansImage*env->tailleMaxFragment;
	} else {
	    tailleFragment = env->tailleMaxFragment;
	}

	sprintf(env->buffer, "%d\r\n%d\r\n%d\r\n%d\r\n", videoClient->id, env->fileSize, env->posDansImage, tailleFragment);

	env->state = SENDING_HEADER;
	env->currentPos = 0;
	env->bufLen = strlen(env->buffer);

	puts("header cree");
	sendHeaderUDP(videoClient);
}

//Envoie le header
void sendHeaderUDP(struct videoClient* videoClient) {
    struct envoi* env = videoClient->envoi;
    
}

//Charge la bonne partie de l'image dans le buffer (memcpy)
void createFragment(struct videoClient* videoClient) {
    struct envoi* env = videoClient->envoi;
    //Mettre à jour la taille bufLen
}

//Envoie le fragment et met à jour la position dans le fichier
void sendFragment(struct videoClient* videoClient) {
    struct envoi* env = videoClient->envoi;
    
}
