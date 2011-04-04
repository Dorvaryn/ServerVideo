#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h> //Pour STDIN_FILENO
#include <pthread.h>

#include "requete.h"
#include "cata.h"
#include "utils.h"
#include "envoi.h"
#include "udp_pull.h"
#include "udp_push.h"
#include "multicast.h"

#define MAX_EVENTS 10
#define BASE_CLIENTS 32

char * catalogue;

void central(int epollfd, struct tabFlux * tabFluxTCP,struct tabFlux * tabFluxUDP)
{
	struct epoll_event events[MAX_EVENTS];

	struct tabClients tabClientsTCP;
	tabClientsTCP.clients =
		(struct sockClient *)malloc(BASE_CLIENTS*sizeof(struct sockClient));
	memset(tabClientsTCP.clients, 0, sizeof(struct sockClient)*BASE_CLIENTS);
	tabClientsTCP.nbClients = 0;


	int baseCouranteTCP = BASE_CLIENTS;
	int nfds;
	int done = 0;

	while(done == 0) //Boucle principale
	{

		nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		FAIL(nfds);

		int n;

		for (n = 0; n < nfds; ++n)
		{
			if (events[n].data.fd == tabFluxTCP->flux[0].sock)
			{
				connectClient(epollfd, &tabClientsTCP, tabFluxTCP, tabFluxTCP->flux[0].sock, &baseCouranteTCP, 1);
			}
			else
			{
				int j = 0;
				int done2 = 0;
				while((done2 == 0) && (j < tabFluxTCP->nbFlux))
				{
					if (events[n].data.fd == tabFluxTCP->flux[j].sock)
					{
						connectClient(epollfd, &tabClientsTCP, tabFluxTCP, tabFluxTCP->flux[j].sock, &baseCouranteTCP, 0);
						done2 = 1;
					}
					else if(events[n].data.fd == STDIN_FILENO)
					{
						char chaine[16];
						fgets(chaine, 15, stdin); //récupère l'entrée standard

						//Traite la commande
						if(strcmp(chaine, "exit\n") == 0 || strcmp(chaine, "quit\n") == 0  || strcmp(chaine, "q\n") == 0)
						{
							done = 1;
							puts("Fin du serveur");
						}
						else
						{
							puts("Commande invalide\nUsage: exit, quit, q");
						}
						done2 = 1;
					}
					j++;
				}
				int i = 0;
				int done3 = 0;
				while((done3 == 0) && (done2 == 0) && (i < tabClientsTCP.nbClients))
				{
					if (events[n].data.fd == tabClientsTCP.clients[i].sock)
					{
						if (events[n].events == (EPOLLIN | EPOLLOUT))
						{					
							if (tabClientsTCP.clients[i].isGET != 1)
							{
								char * buffer = (char *)malloc(512*sizeof(char));
								memset(buffer, '\0', 512);

								recv(tabClientsTCP.clients[i].sock, buffer, 512*sizeof(char),0);
								traiteChaine(buffer, &tabClientsTCP.clients[i].requete, &tabClientsTCP.clients[i].videoClient, 
										epollfd, tabClientsTCP.clients[i].sock);
								free(buffer);
								buffer = NULL;
							}
							else
							{
								send_get_answer(events[n].data.fd, catalogue);
							}
						}
					}
					else if (events[n].data.fd == tabClientsTCP.clients[i].videoClient.clientSocket)
					{
						if(events[n].events == EPOLLOUT)
						{
							if(tabClientsTCP.clients[i].videoClient.clientSocket != 0) 
							{
								sendImage(&tabClientsTCP.clients[i].videoClient, epollfd, tabClientsTCP.clients[i].sock);
							}
						}
						done3 = 1;
					}
					i++;
				}
			}
		}
	} //Fin de la boucle principale

    int i;
	for(i=0; i<tabClientsTCP.nbClients; i++) {
	    free(tabClientsTCP.clients[i].requete.mot);
	    tabClientsTCP.clients[i].requete.mot = NULL;
	    
	    free(tabClientsTCP.clients[i].videoClient.envoi->originBuffer);
	    tabClientsTCP.clients[i].videoClient.envoi->originBuffer = NULL;
	    free(tabClientsTCP.clients[i].videoClient.envoi->buffer);
	    tabClientsTCP.clients[i].videoClient.envoi->buffer = NULL;
	    fclose(tabClientsTCP.clients[i].videoClient.envoi->curFile);
	    
	    free(tabClientsTCP.clients[i].videoClient.envoi);
	    tabClientsTCP.clients[i].videoClient.envoi = NULL;
	    
	    close(tabClientsTCP.clients[i].sock);
	}
	free(tabClientsTCP.clients);
	tabClientsTCP.clients = NULL;

}  

int main(int argc, char ** argv)
{
		struct epoll_event ev;
		memset(&ev, 0, sizeof(struct epoll_event));
		int epollfd;

		epollfd = epoll_create(10);
		FAIL(epollfd);

		//Ajout à epoll de l'entrée standard
		ev.events = EPOLLIN;
		ev.data.fd = STDIN_FILENO;
		FAIL(epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev));

		struct tabFlux tabFluxTCP;
		tabFluxTCP.nbFlux = 0;
		tabFluxTCP.flux = (struct flux *)malloc(BASE_FICHIERS*sizeof(struct flux));

		struct tabFlux tabFluxUDP;
		tabFluxUDP.nbFlux = 0;
		tabFluxUDP.flux = (struct flux *)malloc(BASE_FICHIERS*sizeof(struct flux));

		struct tabFlux tabFluxMCAST;
		tabFluxMCAST.nbFlux = 0;
		tabFluxMCAST.flux = (struct flux *)malloc(BASE_FICHIERS*sizeof(struct flux));


		catalogue = buildCatalogue(epollfd, &tabFluxTCP, &tabFluxUDP, &tabFluxMCAST);

		/*//Lancement du multicast catalogue (ne marche pas pour le moment)
		  pthread_t thread;
		  pthread_create(&thread, NULL, multiCatalogue, (void*)&catalogue);
		  pthread_detach(thread);*/

		int i;
		//Lancement des flux multicast
		for(i = 0; i < tabFluxMCAST.nbFlux; i++) {
			pthread_t thread;
			pthread_create(&thread, NULL, multiFlux, (void*)&tabFluxMCAST.flux[i]);
			pthread_detach(thread);
		}

		for(i = 0; i < tabFluxUDP.nbFlux; i++)
		{
			if(tabFluxUDP.flux[i].infosVideo.type == UDP_PULL)
			{
				//UDP_PULL
				pthread_t thread;
				pthread_create(&thread, NULL, udp_pull, (void*)&tabFluxUDP.flux[i]);
				pthread_detach(thread);
			}
			else if(tabFluxUDP.flux[i].infosVideo.type == UDP_PUSH)
			{
				//UDP_PUSH
				pthread_t thread;
				pthread_create(&thread, NULL, udp_push, (void*)&tabFluxUDP.flux[i]);
				pthread_detach(thread);
			}
		}

		central(epollfd, &tabFluxTCP, &tabFluxUDP);

		int j;
		for(i = 0; i < tabFluxTCP.nbFlux; i++) {
		    close(tabFluxTCP.flux[i].sock);
			for(j = 0; j < BASE_IMAGES; j++) {
				free(tabFluxTCP.flux[i].infosVideo.images[j]);
				tabFluxTCP.flux[i].infosVideo.images[j] = NULL;
			}
			free(tabFluxTCP.flux[i].infosVideo.images);
			tabFluxTCP.flux[i].infosVideo.images = NULL;
		}
		free(tabFluxTCP.flux);
		tabFluxTCP.flux = NULL;

		for(i = 0; i < tabFluxUDP.nbFlux; i++) {
		    close(tabFluxUDP.flux[i].sock);
			for(j = 0; j < BASE_IMAGES; j++) {
				free(tabFluxUDP.flux[i].infosVideo.images[j]);
				tabFluxUDP.flux[i].infosVideo.images[j] = NULL;
			}
			free(tabFluxUDP.flux[i].infosVideo.images);
			tabFluxUDP.flux[i].infosVideo.images = NULL;
		}
		free(tabFluxUDP.flux);
		tabFluxUDP.flux = NULL;
		
		for(i = 0; i < tabFluxMCAST.nbFlux; i++) {
		    close(tabFluxMCAST.flux[i].sock);
			for(j = 0; j < BASE_IMAGES; j++) {
				free(tabFluxMCAST.flux[i].infosVideo.images[j]);
				tabFluxMCAST.flux[i].infosVideo.images[j] = NULL;
			}
			free(tabFluxMCAST.flux[i].infosVideo.images);
			tabFluxMCAST.flux[i].infosVideo.images = NULL;
		}
		free(tabFluxMCAST.flux);
		tabFluxMCAST.flux = NULL;
		
		free(catalogue);
		catalogue = NULL;
		
		close(epollfd);

	return 0;
}
