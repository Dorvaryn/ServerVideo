#include "cata.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

#define FAIL(x) if(x) {\
	perror(#x);}

#define FAIL_FATAL(x) if(x) {\
	perror(#x);exit(EXIT_FAILURE);}

void send_get_answer(int fd, char * catalogue)
{
	puts("Going to send");
	send(fd, catalogue, strlen(catalogue), 0);
	printf("send : %s\n", strerror(errno));
}

int createSockEvent(int epollfd, int port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0); //Version portable des sockets non bloquants
	int flags = fcntl(sock,F_GETFL,O_NONBLOCK); // Version portable des sockets non bloquants
	FAIL(flags);
	FAIL(fcntl(sock,F_SETFL,flags|O_NONBLOCK)); // Version portalble des sockets non bloquants

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
	#if defined ( NEW )
	csock = accept4(sock, (struct sockaddr *)&saddr_client, &size_addr,SOCK_NONBLOCK); //accept4 plus performant
	#endif // NEW

	#if defined ( OLD )
	csock = accept(sock, (struct sockaddr *)&saddr_client, &size_addr);
	int flags = fcntl(csock,F_GETFL,O_NONBLOCK); //Version portalble des sockets non bloquants
	FAIL(flags);
	FAIL(fcntl(csock,F_SETFL,flags|O_NONBLOCK)); //Version portalble des sockets non bloquants
	#endif // OLD

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

void createFichier(int epollfd, struct tabFichiers * tabFichiers, int port, int * baseFichierCourante)
{
	if (tabFichiers->nbFichiers >= *baseFichierCourante)
	{
		*baseFichierCourante *=2;
		int * temp;
		temp = (int *) realloc(tabFichiers->socks,
				*baseFichierCourante*sizeof(int));
		struct infosVideo * temp2;
		temp2 = (struct infosVideo *) realloc(tabFichiers->infosVideos,
				*baseFichierCourante*sizeof(struct infosVideo));
		if (temp!=NULL && temp2!=NULL) 
		{
			tabFichiers->socks = temp;
			tabFichiers->infosVideos = temp2;
		}
		else 
		{
			free (tabFichiers->socks);
			free (tabFichiers->infosVideos);
			puts ("Error (re)allocating memory");
			exit (1);
		}
	}
	tabFichiers->socks[tabFichiers->nbFichiers] = createSockEvent(epollfd,port);
	tabFichiers->infosVideos[tabFichiers->nbFichiers].nbImages = 0;
	tabFichiers->infosVideos[tabFichiers->nbFichiers].images = (char **)malloc(512*sizeof(char)*254);
	tabFichiers->nbFichiers++;
}

void addImage(char * image, struct infosVideo * infos)
{
	strcpy(infos->images[infos->nbImages],image);
	infos->nbImages++;
}

void connectClient(int epollfd, struct tabClients * tabClients, struct tabFichiers * tabFichiers, int sock, int * baseCourante, int isGet)
{
	if (tabClients->nbClients >= *baseCourante)
	{
		*baseCourante *=2;
		struct sockClient * temp;
		temp = (struct sockClient *) realloc(tabClients->clients,
				*baseCourante*sizeof(struct sockClient));
		if (temp!=NULL) 
		{
			tabClients->clients = temp;
		}
		else 
		{
			free (tabClients->clients);
			puts ("Error (re)allocating memory");
			exit (1);
		}
	}
	tabClients->clients[tabClients->nbClients].sock =
		createSockClientEvent(epollfd, sock);
	initReq(&(tabClients->clients[tabClients->nbClients].requete));
	tabClients->clients[tabClients->nbClients].isGET = isGet;
	
	int done = 0;
	int i = 0;
	while((done == 0) && (i < tabFichiers->nbFichiers))
	{
		if(tabFichiers->socks[i] == sock)
		{
			tabClients->sockClient[tabClients->nbClients].videoClient->infos = &tabFichiers->infosVideos[i];
			done = 1;
		}
		i++;
	}
	tabClients->nbClients++;
}

int createEventPull(int epollfd, int csock)
{
	ev.events = EPOLLOUT | EPOLLET;
	ev.data.fd = csock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, csock,
			    &ev) == -1)
	{
	    perror("epoll_ctl: csock");
		exit(EXIT_FAILURE);
	}
}

int createEventPush(int epollfd, int csock)
{
	ev.events = EPOLLOUT;
	ev.data.fd = csock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, csock,
			    &ev) == -1)
	{
	    perror("epoll_ctl: csock");
		exit(EXIT_FAILURE);
	}
}

int connectDataTCP(int epollfd, int sock, int port, int type)
{
	struct sockaddr_in addr, saddr;	
	int csock, len;
	struct epoll_event ev;
	getsockname(sock, (struct sockaddr*)&addr, &len);
	saddr.sin_addr.s_addr = inet_addr(inet_ntoa(addr.sin_addr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	
#if defined ( NEW )
	csock = socket(AF_INET, SOCK_STREAM, SOCK_NONBLOCK); //socket NONBLOCK plus performant 
#endif // NEW
	
#if defined ( OLD )
	csock = socket(AF_INET, SOCK_STREAM, 0);
	int flags = fcntl(csock,F_GETFL,O_NONBLOCK); //Version portalble des sockets non bloquants
	FAIL(flags);
	FAIL(fcntl(csock,F_SETFL,flags|O_NONBLOCK)); //Version portalble des sockets non bloquants
#endif // OLD

	socklen_t size_addr = sizeof(struct sockaddr_in);
	connect(csock, (struct sockaddr *)&saddr, size_addr);
	
	if(type == TCP_PULL)
	{
		createEventPull(epollfd, csock);
	}
	else if(type == TCP_PUSH)
	{
		createEventPush(epollfd, csock);
	}

    return csock;	
}
