#include "multicast.h"
#include "envoi.h"

#define FRAGMENT_SIZE 512

void* multiCatalogue(void* args) {
	//Créer le socket multicast et envoyer le catalogue tous les x secondes
	while(1)
	{
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
	video->envoi->curFile = fopen(video->infosVideo->images[0], "r"); //TODO: initialiser curFile avec le bon fichier
	if(video->envoi->curFile == NULL) {
		puts("E: ouverture du fichier");
	}
	fseek(video->envoi->curFile, 0, SEEK_END);
	video->envoi->fileSize = ftell(video->envoi->curFile);
	fseek(video->envoi->curFile, 0, SEEK_SET);
	printf("socket : %d\n",video->clientSocket);	
	video->envoi->posDansImage = 0;
	video->envoi->tailleMaxFragment = FRAGMENT_SIZE - 128;

	video->id = 1;

	printf("fps: %f\n",flux->infosVideo.fps);
	while(1)
	{
		sendImage(video);
		usleep((1.0/flux->infosVideo.fps)*1000000);
	}

	return NULL;
}