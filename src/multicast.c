#include "multicast.h"
#include "envoi.h"
#include "utils.h"

#define FRAGMENT_SIZE 512

void* multiCatalogue(void* args) {
	//Créer le socket multicast et envoyer le catalogue tous les x secondes
	char * origineBuffer = (char *)args;
	int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	
	struct sockaddr_in dest_addr;
	memset(&dest_addr,0,sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = inet_addr("225.6.7.8");
	dest_addr.sin_port = htons(4567);
	while(1)
	{
		char * buffer = origineBuffer;
		int bufLen = strlen(origineBuffer);
		int nbSent = 0;
		do
		{
			nbSent = sendto(clientSocket, buffer, bufLen, 0,
					(struct sockaddr*)&dest_addr, sizeof(struct sockaddr));
			FAIL(nbSent);

			if( nbSent > 0)
			{
				buffer += nbSent;
				bufLen -= nbSent;
			}

		} while (bufLen > 0);
		sleep(1);
	}

	return NULL;
}

void* multiFlux(void* leflux) {
	//Créer le socket multicast et diffuser le flux

	struct flux * flux = (struct flux *) leflux;
	struct videoClient * video = (struct videoClient *)malloc(sizeof(struct videoClient));
	memset(video, 0, sizeof(video));
	video->clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	video->infosVideo = &flux->infosVideo;

	memset(&video->dest_addr,0,sizeof(&video->dest_addr));
	video->dest_addr.sin_family = AF_INET;
	video->dest_addr.sin_addr.s_addr = inet_addr(flux->adresse);
	video->dest_addr.sin_port = htons(flux->port);

	video->envoi = (struct envoi *)malloc(sizeof(struct envoi));
	video->envoi->state = NOTHING_SENT;
	video->envoi->curFile = fopen(video->infosVideo->images[0], "r");
	if(video->envoi->curFile == NULL) {
		puts("E: ouverture du fichier");
	}
	fseek(video->envoi->curFile, 0, SEEK_END);
	video->envoi->fileSize = ftell(video->envoi->curFile);
	fseek(video->envoi->curFile, 0, SEEK_SET);
	
	video->envoi->posDansImage = 0;
	video->envoi->tailleMaxFragment = FRAGMENT_SIZE - 128;

	video->id = 1;
	video->etat = RUNNING;
	video->dernierEnvoi = -1000; //Il y a très longtemps

	while(1)
	{
		sendImage(video, -1, -1);
		usleep((1.0/flux->infosVideo.fps)*1000000);
	}

	return NULL;
}
