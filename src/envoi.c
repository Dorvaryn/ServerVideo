#include "envoi.h"
#include <errno.h>

double getTime() {
	struct timeval timev;

	gettimeofday(&timev, NULL);
	double micro = (double)timev.tv_usec;
	double sec = (double)timev.tv_sec;

	return sec + micro/10000000;
}

double timeInterval(double t1, double t2) {
    return (t1 < t2 ? t2-t1 : t2); //Evite le bug de minuit :)
}

void sendImage(struct videoClient* videoClient) {

	struct envoi* env = videoClient->envoi;

	if(videoClient->etat != OVER ) //TODO: gérer la pause
	{
		if(videoClient->infosVideo->type == TCP_PULL)
		{
			if(env->state == NOTHING_SENT)
			{
				createHeaderTCP(videoClient);
				sendTCP(videoClient);
			}
			if(env->state == HEADER_SENT) 
			{
				createImageTCP(videoClient);
			}
			if(env->state == SENDING_IMAGE || env->state == SENDING_HEADER) 
			{
				sendTCP(videoClient);
			}
		}
		else if(videoClient->infosVideo->type == TCP_PUSH && videoClient->etat == RUNNING && timeInterval(videoClient->dernierEnvoi, getTime()) >= 1.0/videoClient->infosVideo->fps)
			{
				if(env->state == NOTHING_SENT) 
				{
					createHeaderTCP(videoClient);
					sendTCP(videoClient);
				}
				if(env->state == HEADER_SENT) 
				{
					createImageTCP(videoClient);
				}
				if(env->state == SENDING_IMAGE || env->state == SENDING_HEADER) 
				{
					sendTCP(videoClient);
				}
			}	
		
		else if(videoClient->infosVideo->type == UDP_PULL)
		{
			if(env->state == NOTHING_SENT)
			{
				createHeaderUDP(videoClient);
				sendUDP(videoClient);
			}
			if(env->state == HEADER_SENT) 
			{
				createFragment(videoClient);
			}
			if(env->state == SENDING_FRAGMENT || env->state == SENDING_HEADER) 
			{
				sendUDP(videoClient);
			}
		}
		else if(videoClient->infosVideo->type ==UDP_PUSH)
		{
			//TODO: push udp !
		}
	}

}

void createHeaderTCP(struct videoClient* videoClient) 
{

	struct envoi* env = videoClient->envoi;

	env->buffer = malloc(128*sizeof(char));
	env->originBuffer = env->buffer;
	memset(env->buffer,'\0',128*sizeof(char));

	//Taille
	fseek(env->curFile, 0, SEEK_END);
	env->fileSize = ftell(env->curFile);
	fseek(env->curFile, 0, SEEK_SET);

	sprintf(env->buffer, "%d\r\n%d\r\n", videoClient->id,env->fileSize);

	env->state = SENDING_HEADER;
	env->bufLen = strlen(env->buffer);

	puts("header cree");
}

void createImageTCP(struct videoClient* videoClient) 
{
	struct envoi* env = videoClient->envoi;

	env->buffer = malloc(env->fileSize*sizeof(char));
	env->originBuffer = env->buffer;
	memset(env->buffer,'\0',env->fileSize*sizeof(char));
	env->bufLen = env->fileSize;

	int retour = fread(env->buffer, sizeof(char), env->fileSize, env->curFile);
	if(retour == -1) {
		perror("fread raté");
	}
	env->state = SENDING_IMAGE;
	puts("image cree");
}

void sendTCP(struct videoClient* videoClient) 
{

	struct envoi* env = videoClient->envoi;

	int nbSent;
	do
	{	
		printf("socket du client : %d\n", videoClient->clientSocket);

		nbSent = send(videoClient->clientSocket, env->buffer, env->bufLen, MSG_NOSIGNAL);
		FAIL_SEND(nbSent);

		env->buffer += nbSent;
		env->bufLen -= nbSent;

		printf("car envoyes : %d/%d\n", nbSent, env->bufLen);


	} while (errno != EAGAIN && env->bufLen > 0);

	if(env->bufLen <= 0) 
	{
		if(env->state == SENDING_HEADER)
		{
			env->state = HEADER_SENT;
			free(env->originBuffer);
			puts("Envoyé");
		}
		else if(env->state == SENDING_IMAGE)
		{
			if(videoClient->infosVideo->type == TCP_PUSH)
			{
				fclose(env->curFile);
				free(env->originBuffer);
				videoClient->id = (videoClient->id < videoClient->infosVideo->nbImages ? videoClient->id+1 : 1);
				videoClient->dernierEnvoi = getTime();
				videoClient->envoi->state = NOTHING_SENT;
				videoClient->envoi->curFile = fopen(videoClient->infosVideo->images[videoClient->id-1], "r");
				if(videoClient->envoi->curFile == NULL)
				{
					puts("E: ouverture du fichier");
				}
			}
			else
			{
				env->state = IMAGE_SENT; //Pour le moment on envoie toujours la même image
				fclose(env->curFile);
				free(env->originBuffer);
				puts("Envoyé");
			}
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
	env->bufLen = strlen(env->buffer);

	puts("header cree");
}

//Charge la bonne partie de l'image dans le buffer (memcpy)
void createFragment(struct videoClient* videoClient) {
    //struct envoi* env = videoClient->envoi;
    //Mettre à jour la taille bufLen
}

//Envoie le fragment ou le header suivant l'etat et met à jour la position dans le fichier
void sendUDP(struct videoClient* videoClient) {
    //struct envoi* env = videoClient->envoi;
    
}
