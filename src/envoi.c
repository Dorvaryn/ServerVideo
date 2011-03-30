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

	if(videoClient->etat == RUNNING)
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
		else if(videoClient->infosVideo->type == TCP_PUSH )
			{
				if(env->state == NOTHING_SENT && timeInterval(videoClient->dernierEnvoi, getTime()) >= 1.0/videoClient->infosVideo->fps) 
				{
					videoClient->dernierEnvoi = getTime();
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
			while(env->state != IMAGE_SENT)
			{
				if(env->state == NOTHING_SENT || env->state == FRAGMENT_SENT)
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
		}
		else if(videoClient->infosVideo->type == UDP_PUSH) 
		{
		    if(timeInterval(videoClient->dernierEnvoi, getTime()) >= 1.0/videoClient->infosVideo->fps)
		    {
		        videoClient->dernierEnvoi = getTime();
			    while(env->state != IMAGE_SENT)
			    {
				    if((env->state == NOTHING_SENT || env->state == FRAGMENT_SENT))
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
			}
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

	FAIL(fread(env->buffer, sizeof(char), env->fileSize, env->curFile));
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
		FAIL(nbSent);

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
		}
		else if(env->state == SENDING_IMAGE)
		{
			if(videoClient->infosVideo->type == TCP_PUSH)
			{
				fclose(env->curFile);
				free(env->originBuffer);
				videoClient->id = (videoClient->id < videoClient->infosVideo->nbImages ? videoClient->id+1 : 1);
				videoClient->envoi->state = NOTHING_SENT;
				videoClient->envoi->curFile = fopen(videoClient->infosVideo->images[videoClient->id-1], "r");
				if(videoClient->envoi->curFile == NULL)
				{
					puts("E: ouverture du fichier");
				}
			}
			else
			{
				env->state = IMAGE_SENT;
				fclose(env->curFile);
				free(env->originBuffer);
				puts("Envoyé");
			}
		}
	}
}

//Cree un header pour un paquet
void createHeaderUDP(struct videoClient* videoClient) {
	struct envoi* env = videoClient->envoi;

	env->buffer = malloc(128*sizeof(char));
	memset(env->buffer,'\0',128*sizeof(char));
	env->originBuffer = env->buffer;

	//Taille
	fseek(env->curFile, 0, SEEK_END);
	env->fileSize = ftell(env->curFile);
	fseek(env->curFile, 0, SEEK_SET);


	if(env->fileSize - env->posDansImage < env->tailleMaxFragment) {
		env->tailleFragment = env->fileSize - env->posDansImage;
	} else {
		env->tailleFragment = env->tailleMaxFragment;
	}

	sprintf(env->buffer, "%d\r\n%d\r\n%d\r\n%d\r\n", videoClient->id, env->fileSize, env->posDansImage, env->tailleFragment);

	env->state = SENDING_HEADER;
	env->bufLen = strlen(env->buffer);
	env->more = 1;
	printf(" buffer %s\n",env->buffer);
	printf(" socket %d\n",videoClient->clientSocket);
	puts("header cree");
}

//Charge la bonne partie de l'image dans le buffer (memcpy)
void createFragment(struct videoClient* videoClient) {
	struct envoi* env = videoClient->envoi;

	env->buffer = malloc(env->tailleFragment*sizeof(char));
	env->originBuffer = env->buffer;
	memset(env->buffer,'\0',env->tailleFragment*sizeof(char));
	env->bufLen = env->tailleFragment;

	fseek(env->curFile, env->posDansImage, SEEK_SET);
	FAIL(fread(env->buffer, sizeof(char), env->tailleFragment, env->curFile));

	env->state = SENDING_FRAGMENT;
	env->more = 0;
}

//Envoie le fragment ou le header suivant l'etat et met à jour la position dans le fichier
void sendUDP(struct videoClient* videoClient) {
	struct envoi* env = videoClient->envoi;

	int nbSent = 0;
	do
	{	
		nbSent = sendto(videoClient->clientSocket, env->buffer, env->bufLen, (env->more == 1 ? MSG_MORE : 0),
				(struct sockaddr*)&videoClient->dest_addr, sizeof(struct sockaddr));
		FAIL(nbSent);

		env->buffer += nbSent;
		env->bufLen -= nbSent;

	} while (env->bufLen > 0);

	if(env->bufLen <= 0) //Quand tout est envoyé
	{
		if(env->more == 0)
		{
		    env->state = FRAGMENT_SENT;
			env->posDansImage += env->tailleFragment;
			if(env->posDansImage >= env->fileSize)
			{
				if(videoClient->infosVideo->type == UDP_PUSH)
				{
					videoClient->id = (videoClient->id < videoClient->infosVideo->nbImages ? videoClient->id+1 : 1);
					videoClient->envoi->state = NOTHING_SENT;
					env->posDansImage = 0;
					videoClient->envoi->curFile = fopen(videoClient->infosVideo->images[videoClient->id-1], "r");
					if(videoClient->envoi->curFile == NULL)
					{
						puts("E: ouverture du fichier");
					}
				}
				else
				{
					env->state = IMAGE_SENT;
					fclose(env->curFile);
					free(env->originBuffer);
				}
			}
		}
		else
		{
			env->state = HEADER_SENT;
			free(env->originBuffer);
		}

	}
}

