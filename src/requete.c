#include "requete.h"
#include "envoi.h"

/**
 * Converti un char* en un entier.
 * Le char* doit représenter un nombre entre -1 et la taille max
 * d'un int non signé.
 */
int parseInt(char* entier) {
	if(strcmp(entier, "-1") == 0) {
		return -1;
	}

	int res = 0;
	int i;
	for(i = 0; entier[i] != '\0'; i++) {
		char c = entier[i];
		if(isdigit(c)) {
			res = res*10+(c-'0');
		} else {
			return PARSE_ERROR;
		}
	}

	return res;
}

void initReq(struct requete* req) {
	req->type = NON_DEFINI;

	req->isOver = 0;

	req->imgId = -2;
	req->listenPort = -1;
	req->fragmentSize = -1;

	req->inWord = 0;
	req->space = 0;
	req->crlfCounter = 0;

	req->mot = 0;
	req->motPosition = 0;

	req->reqPosition = 0;
}

void traiteRequete(struct requete* req, struct videoClient* videoClient, int epollfd, int sock) {
	switch(req->type) {
		case BAD_REQUEST:
			puts("E: Mauvaise requete");
			break;
		case GET:
			if(req->fragmentSize != -1) 
			{
				printf("GET id:%d port:%d frag_size:%d\n", req->imgId, req->listenPort, req->fragmentSize);
				
				//videoClient->clientSocket = initDataUDP(epollfd, sock, req->listenPort, UDP_PULL);
				
				videoClient->envoi = malloc(sizeof(struct envoi));
				videoClient->envoi->state = NOTHING_SENT;
				videoClient->envoi->curFile = fopen(videoClient->infosVideo->images[0], "r"); //TODO: initialiser curFile avec le bon fichier
				if(videoClient->envoi->curFile == NULL) {
					puts("E: ouverture du fichier");
				}
				fseek(videoClient->envoi->curFile, 0, SEEK_END);
	            videoClient->envoi->fileSize = ftell(videoClient->envoi->curFile);
	            fseek(videoClient->envoi->curFile, 0, SEEK_SET);
				
				videoClient->envoi->posDansImage = 0;
				videoClient->envoi->tailleMaxFragment = req->fragmentSize;
				
				videoClient->id = 1;


			} else if(req->listenPort != -1) {
				printf("GET id:%d port:%d\n", req->imgId, req->listenPort);

				videoClient->clientSocket = connectDataTCP(epollfd, sock, req->listenPort, videoClient->infosVideo->type);
				printf("socket du client : %d\n", videoClient->clientSocket);

				videoClient->envoi = malloc(sizeof(struct envoi));
				if(videoClient->infosVideo->type == TCP_PUSH)
				{
					videoClient->etat = RUNNING;
					videoClient->envoi->curFile = fopen(videoClient->infosVideo->images[0], "r");
					if(videoClient->envoi->curFile == NULL)
					{
						puts("E: ouverture du fichier");
					}
				}
				else
				{
					videoClient->etat = PAUSE;
				}
				videoClient->dernierEnvoi = getTime();
				videoClient->envoi->state = NOTHING_SENT;
				videoClient->id = 0;
				puts("VIDEO OKok !");


			} else {
				printf("GET id:%d\n", req->imgId);
				videoClient->etat = RUNNING;

				if (req->imgId == -1)
				{	
					videoClient->id = (videoClient->id < videoClient->infosVideo->nbImages ? videoClient->id+1 : 1);
				}
				else
				{
					videoClient->id = req->imgId;
				}
				free(videoClient->envoi);
				videoClient->envoi = malloc(sizeof(struct envoi));
				videoClient->envoi->state = NOTHING_SENT;
				videoClient->envoi->curFile = fopen(videoClient->infosVideo->images[videoClient->id-1], "r");
				if(videoClient->envoi->curFile == NULL)
				{
					puts("E: ouverture du fichier");
				}
				sendImage(videoClient); 
			}
			break;
		case START:
			printf("START\n");
			if(videoClient->etat == PAUSED) 
			{
				struct epoll_event ev;
				ev.events = EPOLLOUT;
				ev.data.fd = videoClient->clientSocket;
				FAIL(epoll_ctl(epollfd, EPOLL_CTL_ADD, videoClient->clientSocket, &ev));
				videoClient->etat = RUNNING;
			}
			break;
		case PAUSE:
			printf("PAUSE\n");
			if(videoClient->etat == RUNNING) 
			{
				struct epoll_event ev;
				ev.events = 0;
				ev.data.fd = videoClient->clientSocket;
				FAIL(epoll_ctl(epollfd, EPOLL_CTL_DEL, videoClient->clientSocket, &ev));
				videoClient->etat = PAUSED;
			}
			break;
		case END:
			printf("END\n");
			struct epoll_event ev;
			ev.events = 0;
			ev.data.fd = videoClient->clientSocket;
			FAIL(epoll_ctl(epollfd, EPOLL_CTL_DEL, videoClient->clientSocket, &ev));
			videoClient->etat = OVER;
			break;
		case ALIVE:
			printf("ALIVE id:%d port:%d\n", req->imgId, req->listenPort);
			videoClient->lastAlive = time(NULL);
			break;
		default:
			break;
	}
}

void traiteChaine(char* chaine, struct requete* req, struct videoClient* videoClient, int epollfd, int sock) {

	if(req->mot == 0) {
		req->mot = malloc(MAX_TOCKEN*sizeof(char));
	}

	puts("==>");
	puts(chaine);
	puts("<==");

	int i;
	for(i=0; chaine[i] != '\0' && !req->isOver; i++) {

		char c = chaine[i];

		//est-ce que le caractère est un espace ?
		req->space = (c == ' ' || c == '\n' || c == '\r');

		if(req->inWord && req->space) { //Le mot est fini, on peut le traiter
			req->mot[req->motPosition] = '\0';
			req->inWord = 0;


			//Traitement du mot lu
			if(req->reqPosition == 0) {
				if(strcmp(req->mot, "GET") == 0) {
					req->type = GET;
				} else if(strcmp(req->mot, "START") == 0) {
					req->type = START;
				} else if(strcmp(req->mot, "PAUSE") == 0) {
					req->type = PAUSE;
				} else if(strcmp(req->mot, "END") == 0) {
					req->type = END;
				} else if(strcmp(req->mot, "ALIVE") == 0) {
					req->type = ALIVE;
				} else {
					req->type = BAD_REQUEST;
				}
			} else if(req->reqPosition == 1) {
				int numero = parseInt(req->mot);
				if(numero == PARSE_ERROR) {
					req->imgId = 0;
				} else {
					req->imgId = numero;
				}
			} else if(req->reqPosition == 2 && strcmp(req->mot, "LISTEN_PORT") != 0) {
				req->type = BAD_REQUEST;
			} else if(req->reqPosition == 3) {
				int numero = parseInt(req->mot);
				if(numero == PARSE_ERROR) {
					req->type = BAD_REQUEST;
				} else {
					req->listenPort = numero;
				}
			} else if(req->reqPosition == 4) {
				if(req->type == ALIVE || strcmp(req->mot, "FRAGMENT_SIZE") != 0) {
					req->type = BAD_REQUEST;
				}
			} else if(req->reqPosition == 5) {
				int numero = parseInt(req->mot);
				if(numero == PARSE_ERROR) {
					req->type = BAD_REQUEST;
				} else {
					req->fragmentSize = numero;
				}
			} else if(req->reqPosition == 6) {
				req->type = BAD_REQUEST;
			}

			req->reqPosition++;


		} else if(req->inWord && !req->space) { //Le mot continue
			req->mot[req->motPosition] = c;
			req->motPosition++;
		} else if(!req->inWord && !req->space) { //Le mot commence
			req->crlfCounter = 0;
			req->motPosition = 0;
			req->inWord = 1;
			req->mot[req->motPosition] = c;
			req->motPosition++;
		} else { //Sinon les espaces continuent
			if(c == '\n') {
				req->crlfCounter++;
			}
			if(req->crlfCounter == 2) {
				req->isOver = 1;
				//puts(req->mot);
			}
		}

	}

	if(req->isOver) {
		free(req->mot);

		if(req->type == GET && req->imgId == -2) {
			req->type = BAD_REQUEST;
		}
		if(req->type == ALIVE && req->imgId == -2) {
			req->type = BAD_REQUEST;
		}
		traiteRequete(req, videoClient, epollfd, sock);
		initReq(req);
	}
}

