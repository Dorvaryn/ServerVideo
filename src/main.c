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
	
	struct tabClients tabClientsUDP;
	tabClientsUDP.clients =
		(struct sockClient *)malloc(BASE_CLIENTS*sizeof(struct sockClient));
	tabClientsUDP.nbClients = 0;

	int baseCouranteTCP = BASE_CLIENTS;
	int baseCouranteUDP = BASE_CLIENTS;
	int nfds;
	int done = 0;

	while(done == 0) //Boucle principale
	{

		nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		FAIL(nfds);
		//puts("working ...");
		
		int n;

		for (n = 0; n < nfds; ++n)
		{
			if (events[n].events != (EPOLLOUT))
			{	
				printf("event : %d : fd : %d\n",events[n].events,events[n].data.fd);
			}
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
						char chaine[16];
						fgets(chaine, 15, stdin); //récupère l'entrée standard

						//Traite la commande
						if(strcmp(chaine, "exit\n") == 0)
						{
							done = 1;
							printf("%s\n", "Fin du serveur");
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
					if (events[n].data.fd == tabFluxUDP->socks[i])
					{
						puts("plop");
						if (events[n].events == EPOLLIN )
						{					
							printf("%s\n", "buffer");
							char * buffer = (char *)malloc(512*sizeof(char));
							memset(buffer, '\0', 512);

							struct sockaddr_in faddr;
							memset(&faddr, 0, sizeof(struct sockaddr_in));
							socklen_t len = sizeof(faddr);
							
							printf("socket : %d\n",tabFluxUDP->socks[i]);
							printf("%s\n", "recvfrom");
							FAIL(recvfrom(tabFluxUDP->socks[i], buffer, 512*sizeof(char),0, (struct sockaddr*)&faddr, &len));

							int j = 0;
							int trouve = -1;
							while((trouve == -1) && (j < tabClientsUDP.nbClients))
							{
								struct sockaddr_in * comp = &tabClientsUDP.clients[j].videoClient.dest_addr;
								if(faddr.sin_addr.s_addr == comp->sin_addr.s_addr 
										&& faddr.sin_family == comp->sin_family
											&& faddr.sin_port == comp->sin_port)
								{
									trouve = j;
								}
								j++;
							}
							if(trouve == -1)
							{
								if (tabClientsUDP.nbClients >= baseCouranteUDP)
								{
									baseCouranteUDP *=2;
									struct sockClient *  temp;
									temp = (struct sockClient *) realloc(tabClientsUDP.clients,
											baseCouranteUDP*sizeof(struct sockClient));
									if (temp!=NULL) 
									{
										tabClientsUDP.clients = temp;
									}
									else 
									{
										free (tabClientsUDP.clients);
										puts ("Error (re)allocating memory");
										exit (1);
									}
								}
								initReq(&(tabClientsUDP.clients[tabClientsUDP.nbClients].requete));
								memset(&tabClientsUDP.clients[tabClientsUDP.nbClients].videoClient.dest_addr,0,sizeof(struct sockaddr));
								memcpy(&tabClientsUDP.clients[tabClientsUDP.nbClients].videoClient.dest_addr, &faddr, sizeof(struct sockaddr));
								tabClientsUDP.clients[tabClientsUDP.nbClients].videoClient.clientSocket = tabFluxUDP->socksData[i];
								tabClientsUDP.clients[tabClientsUDP.nbClients].videoClient.infosVideo = &tabFluxUDP->infosVideos[i];
								trouve = tabClientsUDP.nbClients++;
							}
							traiteChaine(buffer, &tabClientsUDP.clients[trouve].requete, &tabClientsUDP.clients[trouve].videoClient, 
									epollfd, 0);
							printf("%s\n", "done");
							done4 = 1;
						}

					}
					i++;
				}
				i = 0;
				int done5 = 0;
				while((done5 == 0) && (done4 == 0) && (done3 == 0) && (done2 == 0) && (i < tabClientsUDP.nbClients))
				{
					if (events[n].data.fd == tabClientsUDP.clients[i].videoClient.clientSocket)
					{
						if(events[n].events == EPOLLOUT)
						{
								printf("%s\n","ENVOI");
								printf("%d\n", tabClientsUDP.clients[i].videoClient.clientSocket);
								if(tabClientsUDP.clients[i].videoClient.clientSocket != 0) 
								{
									sendImage(&tabClientsUDP.clients[i].videoClient);
								}
						}
						done5 = 1;
					}
					i++;
				}
			}
		}
	} //Fin de la boucle principale

	//TODO: fermer les ressources proprement ici
	free(tabClientsTCP.clients);

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
	tabFluxTCP.socks = (int *)malloc(BASE_FICHIERS*sizeof(int));
	tabFluxTCP.infosVideos = (struct infosVideo *) malloc(BASE_FICHIERS*sizeof(struct infosVideo));
	
	struct tabFlux tabFluxUDP;
	tabFluxUDP.nbFlux = 0;
	tabFluxUDP.socks = (int *)malloc(BASE_FICHIERS*sizeof(int));
	tabFluxUDP.socksData = (int *)malloc(BASE_FICHIERS*sizeof(int));
	tabFluxUDP.infosVideos = (struct infosVideo *) malloc(BASE_FICHIERS*sizeof(struct infosVideo));

	int baseFichiersCourante = BASE_FICHIERS;
	createFichier(epollfd, &tabFluxTCP, 8081, &baseFichiersCourante, 0);

	catalogue = buildCatalogue(epollfd, &tabFluxTCP, &tabFluxUDP);

	central(epollfd, &tabFluxTCP, &tabFluxUDP);

    int i, j;
    for(i = 0; i < tabFluxTCP.nbFlux; i++) {
        for(j = 0; j < BASE_IMAGES; j++) {
            free(tabFluxTCP.infosVideos[i].images[j]);
        }
        free(tabFluxTCP.infosVideos[i].images);
    }
    free(tabFluxTCP.infosVideos);
    for(i = 0; i < tabFluxUDP.nbFlux; i++) {
        for(j = 0; j < BASE_IMAGES; j++) {
            free(tabFluxUDP.infosVideos[i].images[j]);
        }
        free(tabFluxUDP.infosVideos[i].images);
    }
    free(tabFluxUDP.infosVideos);    

    free(tabFluxUDP.socks);
    free(tabFluxTCP.socks);
	free(catalogue);

	return 0;
}
