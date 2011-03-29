#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h> //Pour STDIN_FILENO

#include "requete.h"
#include "cata.h"
#include "utils.h"
#include "envoi.h"

#define MAX_EVENTS 10
#define BASE_CLIENTS 32

char * catalogue;

void central(int epollfd, struct tabFlux * tabFluxTCP,struct tabFlux * tabFluxUDP)
{
	struct epoll_event events[MAX_EVENTS];

	struct tabClients tabClientsTCP;
	tabClientsTCP.clients =
		(struct sockClient *)malloc(BASE_CLIENTS*sizeof(struct sockClient));
	tabClientsTCP.nbClients = 0;

	int baseCouranteTCP = BASE_CLIENTS;
	//int baseCouranteUDP = BASE_CLIENTS;
	int nfds;
	int done = 0;

	while(done == 0) //Boucle principale
	{

		nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		FAIL(nfds);
		puts("working ...");
		
		int n;

		for (n = 0; n < nfds; ++n)
		{
			printf("%s\n", "event");
			printf("%d\n", events[n].events);
			if (events[n].data.fd == tabFluxTCP->socks[0])
			{
				printf("%s\n", "connect8081");
				connectClient(epollfd, &tabClientsTCP, tabFluxTCP, tabFluxTCP->socks[0], &baseCouranteTCP, 1);
			}
			else
			{
				int j = 0;
				int done2 = 0;
				while((done2 == 0) && (j < tabFluxTCP->nbFlux))
				{
					if (events[n].data.fd == tabFluxTCP->socks[j])
					{
						printf("%s\n", "connectOther");
						connectClient(epollfd, &tabClientsTCP, tabFluxTCP, tabFluxTCP->socks[j], &baseCouranteTCP, 0);
						done2 = 1;
					}
					else if(events[n].data.fd == STDIN_FILENO)
					{
						char chaine[512];
						scanf("\n%s", chaine); //récupère l'entrée standart

						//Traite la commande
						printf("%s\n", "sort");
						if(strcmp(chaine, "exit") == 0)
						{
							done = 1;
							printf("%s\n", "done = 1");
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
								printf("%s\n", "buffer");
								char * buffer = (char *)malloc(512*sizeof(char));
								memset(buffer, '\0', 512);

								printf("%s\n", "recv");
								recv(tabClientsTCP.clients[i].sock, buffer, 512*sizeof(char),0);
								traiteChaine(buffer, &tabClientsTCP.clients[i].requete, &tabClientsTCP.clients[i].videoClient, 
										epollfd, tabClientsTCP.clients[i].sock);
								printf("%s\n", "done");
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
								//TODO: vérifier que cette fonction n'est pas appellée au mauvais moment (catalogue)
								printf("%s\n","ENVOI");
								printf("%d\n", tabClientsTCP.clients[i].videoClient.clientSocket);
								if(tabClientsTCP.clients[i].videoClient.clientSocket != 0) 
								{
									sendImage(&tabClientsTCP.clients[i].videoClient);
								}
						}
						done3 = 1;
					}
					i++;
				}
				i = 0;
				int done4 = 0;
				while((done4 == 0) && (done3 == 0) && (done2 == 0) && (i < tabFluxUDP->nbFlux))
				{

				}
			}
		}
	} //Fin de la boucle principale

	//TODO: fermer les ressources proprement ici

}  

int main(int argc, char ** argv)
{
	struct epoll_event ev;
	int epollfd;

	epollfd = epoll_create(10);
	FAIL(epollfd);

	//Ajout à epoll de l'entrée standard
	ev.events = EPOLLIN;
	ev.data.fd = STDIN_FILENO;
	FAIL(epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev));

	struct tabFlux tabFluxTCP;
	tabFluxTCP.nbFlux = 0;
	tabFluxTCP.socks = (int *)malloc(BASE_FICHIERS*sizeof(int));
	tabFluxTCP.infosVideos = (struct infosVideo *) malloc(BASE_FICHIERS*sizeof(struct infosVideo));
	
	struct tabFlux tabFluxUDP;
	tabFluxUDP.nbFlux = 0;
	tabFluxUDP.socks = (int *)malloc(BASE_FICHIERS*sizeof(int));
	tabFluxUDP.infosVideos = (struct infosVideo *) malloc(BASE_FICHIERS*sizeof(struct infosVideo));

	int baseFichiersCourante = BASE_FICHIERS;
	createFichier(epollfd, &tabFluxTCP, 8081, &baseFichiersCourante, 0);

	catalogue = buildCatalogue(epollfd, &tabFluxTCP, &tabFluxUDP);

	central(epollfd, &tabFluxTCP, &tabFluxUDP);

	free(catalogue);

	return 0;
}
