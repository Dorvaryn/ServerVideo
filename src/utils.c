#include "utils.h"
#include <stdlib.h>
#include <string.h>

void send_get_answer(int fd, char * catalogue)
{
	puts("Going to send");
	FAIL(send(fd, catalogue, strlen(catalogue), 0));
}

int createSockEventTCP(int epollfd, int port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0); //Version portable des sockets non bloquants
	FAIL(sock);
	
	int flags = fcntl(sock,F_GETFL,O_NONBLOCK); // Version portable des sockets non bloquants
	FAIL(flags);
	FAIL(fcntl(sock,F_SETFL,flags|O_NONBLOCK)); // Version portalble des sockets non bloquants

	struct sockaddr_in saddr;

	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);

	FAIL(bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)));

	FAIL(listen(sock, 10));

	struct epoll_event ev;
	memset(&ev, 0, sizeof(struct epoll_event));

	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = sock;
	FAIL(epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev));
	
	puts("epolladd");

	return sock;
}

int createSockEventUDP(int epollfd, int port)
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0); //Version portable des sockets non bloquants
	FAIL(sock);

	int flags = fcntl(sock,F_GETFL,O_NONBLOCK); // Version portable des sockets non bloquants
	FAIL(flags);
	FAIL(fcntl(sock,F_SETFL,flags|O_NONBLOCK)); // Version portalble des sockets non bloquants

	struct sockaddr_in saddr;

	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);

	FAIL(bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)));

	struct epoll_event ev;
	memset(&ev, 0, sizeof(struct epoll_event));

	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = sock;
	FAIL(epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev));
	
	puts("epolladd");

	return sock;
}

int createSockClientEvent(int epollfd, int sock)
{
	int csock;
	struct epoll_event ev;
	memset(&ev, 0, sizeof(struct epoll_event));
	struct sockaddr_in saddr_client;

	socklen_t size_addr = sizeof(struct sockaddr_in);
	#if defined ( NEW )
	csock = accept4(sock, (struct sockaddr *)&saddr_client, &size_addr,SOCK_NONBLOCK); //accept4 plus performant
	#endif // NEW

	#if defined ( OLD )
	csock = accept(sock, (struct sockaddr *)&saddr_client, &size_addr);
	FAIL(csock)

	int flags = fcntl(csock,F_GETFL,O_NONBLOCK); //Version portalble des sockets non bloquants
	FAIL(flags);
	FAIL(fcntl(csock,F_SETFL,flags|O_NONBLOCK)); //Version portalble des sockets non bloquants
	#endif // OLD

	printf("Connection de %s :: %d\n", inet_ntoa(saddr_client.sin_addr), 
			htons(saddr_client.sin_port));

	ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
	ev.data.fd = csock;
	FAIL(epoll_ctl(epollfd, EPOLL_CTL_ADD, csock, &ev));
	puts("epolladd");

	return csock;
}

void createFichier(int epollfd, struct tabFlux * tabFlux, int port, int * baseFichierCourante, int type)
{
	if (tabFlux->nbFlux >= *baseFichierCourante)
	{
		*baseFichierCourante *=2;
		struct flux * temp;
		temp = (struct flux *) realloc(tabFlux->flux,
				*baseFichierCourante*sizeof(struct flux));
		
		if (temp!=NULL) 
		{
			tabFlux->flux = temp;
		}
		else 
		{
			free (tabFlux->flux);
			puts ("Error (re)allocating memory");
			exit (1);
		}
	}
	if( type == TCP_PULL || type == TCP_PUSH )
	{
		tabFlux->flux[tabFlux->nbFlux].sock = createSockEventTCP(epollfd,port);
	}
	tabFlux->flux[tabFlux->nbFlux].port = port;
	tabFlux->flux[tabFlux->nbFlux].infosVideo.nbImages = 0;
	tabFlux->flux[tabFlux->nbFlux].infosVideo.images = (char **)malloc(BASE_IMAGES*sizeof(char*));
	int k;
	for (k = 0; k < BASE_IMAGES; k++)
	{
		tabFlux->flux[tabFlux->nbFlux].infosVideo.images[k] = (char *)malloc(512*sizeof(char));
	}
	tabFlux->nbFlux++;
}

void addImage(char * uneImage, struct infosVideo * infos)
{
	printf("Debug precopy\n");
	strcpy(infos->images[infos->nbImages], uneImage);
	printf("Debug postcopy\n");
	infos->nbImages++;
}

void connectClient(int epollfd, struct tabClients * tabClients, struct tabFlux * tabFlux, int sock, int * baseCourante, int isGet)
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
	while((done == 0) && (i < tabFlux->nbFlux))
	{
		if(tabFlux->flux[i].sock == sock)
		{
			tabClients->clients[tabClients->nbClients].videoClient.infosVideo = &tabFlux->flux[i].infosVideo;
			done = 1;
		}
		i++;
	}
	tabClients->nbClients++;
}

void createEventPull(int epollfd, int csock)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(struct epoll_event));
	ev.events = EPOLLOUT | EPOLLET;
	ev.data.fd = csock;
	FAIL(epoll_ctl(epollfd, EPOLL_CTL_ADD, csock, &ev));
}

void createEventPush(int epollfd, int csock)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(struct epoll_event));
	ev.events = EPOLLOUT;
	ev.data.fd = csock;
	FAIL(epoll_ctl(epollfd, EPOLL_CTL_ADD, csock, &ev));
}


int connectDataTCP(int epollfd, int sock, int port, int type)
{
	struct sockaddr_in addr, saddr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	int csock;
	socklen_t len;
	memset(&len, 0, sizeof(socklen_t));
	getsockname(sock, (struct sockaddr*)&addr, &len);
	saddr.sin_addr.s_addr = addr.sin_addr.s_addr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	
#if defined ( NEW )
	csock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK,0); //socket NONBLOCK plus performant 
#endif // NEW
	
#if defined ( OLD )
	csock = socket(AF_INET, SOCK_STREAM, 0);
	int flags = fcntl(csock,F_GETFL,O_NONBLOCK); //Version portalble des sockets non bloquants
	FAIL(flags);
	FAIL(fcntl(csock,F_SETFL,flags|O_NONBLOCK)); //Version portalble des sockets non bloquants
#endif // OLD
	FAIL(csock);

	socklen_t size_addr = sizeof(struct sockaddr_in);
	FAIL(connect(csock, (struct sockaddr *)&saddr, size_addr));
	
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
