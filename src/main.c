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
#define MAX_HEADER 200
#define MAX_STR 32
#define BASE_CLIENTS 32
#define BASE_FICHIERS 32 


void central()
{
	struct epoll_event ev, events[MAX_EVENTS];
	int sock, socktest, nfds, epollfd;

	epollfd = epoll_create(10);
	if (epollfd == -1) {
		perror("epoll_create");
		exit(EXIT_FAILURE);
	}

	sock = createSockEvent(epollfd, 8081);

	socktest = createSockEvent(epollfd, 8087);

	//Ajout à epoll de l'entrée standard
	ev.events = EPOLLIN;
	ev.data.fd = STDIN_FILENO;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
		perror("epoll_ctl: stdin");
		exit(EXIT_FAILURE);
	}

	int done = 0;


	struct tabFichiers tabFichiers;
	tabFichiers.nbFichiers = 0;
	tabFichiers.socks = (int *)malloc(BASE_FICHIERS*sizeof(int));

	struct tabClients tabClients;
	tabClients.clients =
		(struct sockClient *)malloc(BASE_CLIENTS*sizeof(struct sockClient));
	tabClients.nbClients = 0;

	int baseCourante = BASE_CLIENTS;

	while(!done) //Boucle principale
	{

		nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		puts("working ...");
		if (nfds == -1) {
			perror("epoll_pwait");
			exit(EXIT_FAILURE);
		}
		int n,csock,csocktest;
		for (n = 0; n < nfds; ++n) {
			if (events[n].data.fd == sock) 
			{
				connectClient(epollfd, &tabClients, sock, baseCourante, 1);
			}
			else if (events[n].data.fd == socktest)
			{
				connectClient(epollfd, &tabClients, socktest, baseCourante, 0);
			}
			else if(events[n].data.fd == STDIN_FILENO)
			{
				char chaine[512];
				scanf("\n%s", chaine); //récupère l'entrée standart

				//Traite la commande
				if(strcmp(chaine, "exit") == 0)
				{
					done = 1;
				}            
			}
			else 
			{
				int i;
				for(i =0 ; i < tabClients.nbClients; i++)
				{
					if (events[n].data.fd == tabClients.clients[i].sock)
					{
						if (events[n].events == (EPOLLIN | EPOLLOUT))
						{					
							if (tabClients.clients[i].isGET != 1)
							{
								printf("%s\n", "buffer");
								char buffer[512];
								printf("%s\n", "recv");
								recv(tabClients.clients[i].sock, buffer, 512*sizeof(char),0);
								printf("%s\n", "traiteChaine");
								traiteChaine(buffer, &tabClients.clients[i].requete);
								printf("%s\n", "done");
							}
							else
							{
								send_get_answer(events[n].data.fd);
							}
						}
						else if(events[n].events == EPOLLOUT)
						{
							printf("%s\n","TODO: ENVOIE");
						}
					}
				}
			}
		}
	} //Fin de la boucle principale

	//TODO: fermer les ressources proprement ici

}  

int main(int argc, char ** argv)
{
	central();
	return 0;
}
