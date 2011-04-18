#include "envoi.h"
#include "utils.h"
#include <errno.h>

void sendImage(struct videoClient* videoClient, int epollfd, int sockControl) {

	struct envoi* env = videoClient->envoi;
	
	if((videoClient->infosVideo->type == TCP_PUSH 
            || videoClient->infosVideo->type == UDP_PUSH)
            && timeInterval(videoClient->lastAlive, getTime()) >= 60)
	{
		if(videoClient->clientSocket != -1)
		{
			puts("Client mort !");
			decoClient(videoClient, sockControl, epollfd, videoClient->infosVideo->type);
		}
	}

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
		else if(videoClient->infosVideo->type == TCP_PUSH)
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
		else if(videoClient->infosVideo->type == UDP_PUSH || videoClient->infosVideo->type == MCAST_PUSH)  
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
				env->state = NOTHING_SENT;
			}
		}
	}

}

void createHeaderTCP(struct videoClient* videoClient) 
{

	struct envoi* env = videoClient->envoi;

	env->originBuffer = malloc(128*sizeof(char));
	memset(env->originBuffer,'\0',128*sizeof(char));
	env->buffer = env->originBuffer;

	//Taille
	fseek(env->curFile, 0, SEEK_END);
	env->fileSize = ftell(env->curFile);
	fseek(env->curFile, 0, SEEK_SET);

	sprintf(env->buffer, "%d\r\n%d\r\n", videoClient->id,env->fileSize);

	env->state = SENDING_HEADER;
	env->bufLen = strlen(env->buffer);

}

void createImageTCP(struct videoClient* videoClient) 
{
	struct envoi* env = videoClient->envoi;

	env->originBuffer = malloc(env->fileSize*sizeof(char));
	memset(env->originBuffer,'\0',env->fileSize*sizeof(char));
	env->buffer = env->originBuffer;
	env->bufLen = env->fileSize;

	FAIL(fread(env->buffer, sizeof(char), env->fileSize, env->curFile));
	env->state = SENDING_IMAGE;
}

void sendTCP(struct videoClient* videoClient) 
{

	struct envoi* env = videoClient->envoi;

	int nbSent;
	do
	{	
		nbSent = send(videoClient->clientSocket, env->buffer, env->bufLen, MSG_NOSIGNAL);
		FAIL(nbSent);
		
		if( nbSent > 0 )
		{
			env->buffer += nbSent;
			env->bufLen -= nbSent;
		}

	} while (nbSent != -1 && env->bufLen > 0);

	if(env->bufLen <= 0) 
	{
		free(env->originBuffer);
		env->originBuffer = NULL;
		env->buffer = NULL;
		if(env->state == SENDING_HEADER)
		{
			env->state = HEADER_SENT;
		}
		else if(env->state == SENDING_IMAGE)
		{
			if(videoClient->infosVideo->type == TCP_PUSH)
			{
				fclose(env->curFile);
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
				env->curFile = NULL;
			}
		}
	}
}

//Cree un header pour un paquet
void createHeaderUDP(struct videoClient* videoClient) {
	struct envoi* env = videoClient->envoi;

	env->originBuffer = malloc(128*sizeof(char));
	memset(env->originBuffer,'\0',128*sizeof(char));
	env->buffer = env->originBuffer;

	//Taille
	FAIL_NULL(fseek(env->curFile, 0, SEEK_END));
	env->fileSize = ftell(env->curFile);
	FAIL(env->fileSize);
	FAIL_NULL(fseek(env->curFile, 0, SEEK_SET));


	if(env->fileSize - env->posDansImage < env->tailleMaxFragment) {
		env->tailleFragment = env->fileSize - env->posDansImage;
	} else {
		env->tailleFragment = env->tailleMaxFragment;
	}

	sprintf(env->buffer, "%d\r\n%d\r\n%d\r\n%d\r\n", videoClient->id, env->fileSize, env->posDansImage, env->tailleFragment);

	env->state = SENDING_HEADER;
	env->bufLen = strlen(env->buffer);
	env->more = 1;
}

//Charge la bonne partie de l'image dans le buffer (memcpy)
void createFragment(struct videoClient* videoClient) {
	struct envoi* env = videoClient->envoi;

	env->originBuffer = malloc(env->tailleFragment*sizeof(char));
	memset(env->originBuffer,'\0',env->tailleFragment*sizeof(char));
	env->buffer = env->originBuffer;
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

		if( nbSent > 0)
		{
			env->buffer += nbSent;
			env->bufLen -= nbSent;
		}
	} while (nbSent != -1 && env->bufLen > 0);

	if(env->bufLen <= 0) //Quand tout est envoyé
	{
		free(env->originBuffer);
		env->originBuffer = NULL;
		env->buffer = NULL;
		if(env->more == 0)
		{
		    env->state = FRAGMENT_SENT;
			env->posDansImage += env->tailleFragment;
			if(env->posDansImage >= env->fileSize)
			{
				env->state = IMAGE_SENT;
				fclose(env->curFile);
				if(videoClient->infosVideo->type == UDP_PUSH || videoClient->infosVideo->type == MCAST_PUSH)
				{
					videoClient->id = (videoClient->id < videoClient->infosVideo->nbImages ? videoClient->id+1 : 1);
					env->posDansImage = 0;
					videoClient->envoi->curFile = fopen(videoClient->infosVideo->images[videoClient->id-1], "r");
					if(videoClient->envoi->curFile == NULL)
					{
						puts("E: ouverture du fichier");
					}
				}
			}
		}
		else
		{
			env->state = HEADER_SENT;
		}

	}
}

