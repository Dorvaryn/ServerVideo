#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h> //Pour STDIN_FILENO

#include "requete.c"
#include "cata.c"

#define MAX_EVENTS 10
#define MAX_HEADER 200
#define MAX_STR 32
#define BASE_CLIENT 32

char * build_date()
{
	return "00 / 00 / 00";
}

char * build_http_header(char * type, int size)
{
	char * header = malloc(MAX_HEADER * sizeof(char));
	memset(header, '\0', MAX_HEADER);
	strcat(header, "HTTP/1.1 200 OK\r\nDate: ");

	/*Insertion de la date actuelle*/
	strcat(header, build_date());

	strcat(header, "\r\nServer: ServLib (Unix) (Arch/Linux)\r\nAccept-Ranges: bytes\r\nContent-Length: ");

	/*Insertion de la taille*/
	char tmp[MAX_STR] = {'\0'};
	sprintf(tmp, "%d", size);
	strcat(header, tmp);

	strcat(header, "\r\nConnection: close\r\nContent-Type: ");

	/*Insertion du type*/
	strcat(header, type);

	strcat(header, "; charset=UTF-8\r\n\r\n");

	return header;
}

void file_to_buffer(char ** buff, int * size)
{
	/*Ouverture du fichier*/
	FILE * f = fopen("catalogue.txt", "r");
	printf("fopen : %s\n", strerror(errno));

	/*Deplacement du curseur à la fin du fichier*/
	fseek(f, 0,SEEK_END);
	printf("fseek : %s\n", strerror(errno));

	/*On recupere le nombre de caractere*/
	*size = ftell(f);
	printf("fopen : %s\n", strerror(errno));

	int c;
	int i;
	/*On alloue un espace memoire de la taille du fichier*/
	*buff = malloc(*size * sizeof(char));

	/*On se replace au debut du fichier*/
	fseek(f, 0, SEEK_SET);
	printf("fseek : %s\n", strerror(errno));

	/*On recupere tout les caracteres*/
	for(i = 0; i < *size; i++)
	{
		c = fgetc(f);
		(*buff)[i] = c;
	}

	/*Et on referme le fichier*/
	fclose(f);
}

void send_get_answer(int fd)
{
	int size;
	char * buf;
	char * header;
	/*On recuper le fichier sous forme de chaine de cara*/
	buf = buildCatalogue("catalogue.txt");
	/*On construit l'entete HTML aproprié*/
	header = build_http_header("text/plain", strlen(buf));

	printf("%s\n",buf);

	/*Et on envoie les données*/
	puts("Going to send");
	send(fd, header, strlen(header), MSG_MORE);
	printf("send : %s\n", strerror(errno));
	send(fd, buf, strlen(buf), 0);
	printf("send : %s\n", strerror(errno));

	/*On libere les ressources alloué*/
	int l = strlen(buf);
	int i;
	for(i=0;i<l;i++)
	{
		buf[i] = '\0';
	}
	int l2 = strlen(header);
	for(i=0;i<l2;i++)
	{
		header[i] = '\0';
	}
	free(header);
	free(buf);
}

int createSockEvent(int epollfd, int port)
{
	int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	printf("socket : %s\n", strerror(errno));

	struct sockaddr_in saddr;

	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);

	bind(sock, (struct sockaddr *)&saddr, sizeof(saddr));
	printf("bind : %s\n", strerror(errno));

	listen(sock, 10);
	printf("listen : %s\n", strerror(errno));

	struct epoll_event ev;

	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = sock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) 
	{
		perror("epoll_ctl: sock");
		exit(EXIT_FAILURE);
	}

	return sock;
}

int createSockClientEvent(int epollfd, int sock)
{
	int csock;
	struct epoll_event ev;
	struct sockaddr_in saddr_client;

	socklen_t size_addr = sizeof(struct sockaddr_in);
	csock = accept(sock, (struct sockaddr *)&saddr_client, &size_addr);
	printf("accept : %s\n", strerror(errno));

	printf("Connection de %s :: %d\n", inet_ntoa(saddr_client.sin_addr), 
			htons(saddr_client.sin_port));
	if (csock == -1)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}

	ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
	ev.data.fd = csock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, csock,
				&ev) == -1)
	{
		perror("epoll_ctl: csock");
		exit(EXIT_FAILURE);
	}

	return csock;
}

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

	struct sockClient {
		int sock;
		int isGET;
		struct requete requete;
	};
	struct tabClients {
		int nbClients;
		struct sockClient * clients;
	};

	struct tabClients tabClients;
	tabClients.clients =
		(struct sockClient *)malloc(BASE_CLIENT*sizeof(struct sockClient));
	tabClients.nbClients = 0;

	int baseCourante = BASE_CLIENT;

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
				if (tabClients.nbClients >= baseCourante)
				{
					baseCourante *=2;
					struct sockClient * temp;
					temp = (struct sockClient *) realloc(tabClients.clients,
							baseCourante*sizeof(struct sockClient));
					if (temp!=NULL) 
					{
						tabClients.clients = temp;
					}
					else 
					{
						free (tabClients.clients);
						puts ("Error (re)allocating memory");
						exit (1);
					}
				}
				tabClients.clients[tabClients.nbClients].sock =
					createSockClientEvent(epollfd, sock);
				initReq(&tabClients.clients[tabClients.nbClients].requete);
				tabClients.clients[tabClients.nbClients].isGET = 1;
				tabClients.nbClients++;
			}
			else if (events[n].data.fd == socktest)
			{
				if (tabClients.nbClients >= baseCourante)
				{
					baseCourante *=2;
					struct sockClient * temp;
					temp = (struct sockClient *) realloc(tabClients.clients,
							baseCourante*sizeof(struct sockClient));
					if (temp!=NULL) 
					{
						tabClients.clients = temp;
					}
					else 
					{
						free (tabClients.clients);
						puts ("Error (re)allocating memory");
						exit (1);
					}
				}
				tabClients.clients[tabClients.nbClients].sock =
					createSockClientEvent(epollfd, socktest);
				initReq(&tabClients.clients[tabClients.nbClients].requete);
				tabClients.clients[tabClients.nbClients].isGET = 0;
				tabClients.nbClients++;

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
