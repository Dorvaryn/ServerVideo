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

#define MAX_EVENTS 10
#define BASE_CLIENTS 32

char * catalogue;

void central(int epollfd, struct tabFichiers * tabFichiers)
{
	struct epoll_event events[MAX_EVENTS];

	struct tabClients tabClients;
	tabClients.clients =
		(struct sockClient *)malloc(BASE_CLIENTS*sizeof(struct sockClient));
	tabClients.nbClients = 0;

	int baseCourante = BASE_CLIENTS;
	int nfds;
	int done = 0;

	while(done == 0) //Boucle principale
	{

		nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		puts("working ...");
		if (nfds == -1)
	   	{
			perror("epoll_pwait");
			exit(EXIT_FAILURE);
		}
		int n,csock,csocktest;
		
		for (n = 0; n < nfds; ++n)
		{
			printf("%s\n", "action");
			printf("%d\n", events[n].data.fd);
			if (events[n].data.fd == (*tabFichiers).socks[0])
			{
				printf("%s\n", "connect8081");
				connectClient(epollfd, &tabClients, (*tabFichiers).socks[0], &baseCourante, 1);
			}
			else
			{
				int j = 0;
				int done2 = 0;
				while((done2 == 0) && (j < (*tabFichiers).nbFichiers))
				{
					if (events[n].data.fd == (*tabFichiers).socks[j])
					{
						printf("%s\n", "connectOther");
						connectClient(epollfd, &tabClients, (*tabFichiers).socks[j], &baseCourante, 0);
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
				while((done3 == 0) && (done2 == 0) && (i < (*tabFichiers).nbFichiers))
				{
					if (events[n].data.fd == tabClients.clients[i].sock)
					{
						if (events[n].events == (EPOLLIN | EPOLLOUT))
						{					
							if (tabClients.clients[i].isGET != 1)
							{
								printf("%s\n", "buffer");
								char * buffer = (char *)malloc(512*sizeof(char));
								memset(buffer, '\0', 512);

								printf("%s\n", "recv");
								recv(tabClients.clients[i].sock, buffer, 512*sizeof(char),0);
								traiteChaine(buffer, &tabClients.clients[i].requete);
								printf("%s\n", "done");
							}
							else
							{
									send_get_answer(events[n].data.fd, catalogue);
							}
						}
						else if(events[n].events == EPOLLOUT)
						{
							printf("%s\n","TODO: ENVOIE");
						}
						done3 = 1;
					}
					i++;
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
	if (epollfd == -1) {
		perror("epoll_create");
		exit(EXIT_FAILURE);
	}
	//Ajout à epoll de l'entrée standard
	ev.events = EPOLLIN;
	ev.data.fd = STDIN_FILENO;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
		perror("epoll_ctl: stdin");
		exit(EXIT_FAILURE);
	}

	struct tabFichiers tabFichiers;
	tabFichiers.nbFichiers = 0;
	tabFichiers.socks = (int *)malloc(BASE_FICHIERS*sizeof(int));

	int baseFichierCourante = BASE_FICHIERS;
	createFichier(epollfd, &tabFichiers, 8081, &baseFichierCourante);
	
	catalogue = buildCatalogue(epollfd, &tabFichiers);

	central(epollfd, &tabFichiers);

	free(catalogue);

	return 0;
}
