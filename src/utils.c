#include "cata.h"
#include "utils.h"

void send_get_answer(int fd, char * catalogue)
{
	puts("Going to send");
	send(fd, catalogue, strlen(catalogue), 0);
	printf("send : %s\n", strerror(errno));
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

void createFichier(int epollfd, struct tabFichiers * tabFichiers, int port, int * baseFichierCourante)
{
	if (tabFichiers->nbFichiers >= *baseFichierCourante)
	{
		*baseFichierCourante *=2;
		int * temp;
		temp = (int *) realloc(tabFichiers->socks,
				*baseFichierCourante*sizeof(int));
		if (temp!=NULL) 
		{
			tabFichiers->socks = temp;
		}
		else 
		{
			free (tabFichiers->socks);
			puts ("Error (re)allocating memory");
			exit (1);
		}
	}
	tabFichiers->socks[tabFichiers->nbFichiers] = createSockEvent(epollfd,port);
	tabFichiers->nbFichiers++;
}

void connectClient(int epollfd, struct tabClients * tabClients, int sock, int * baseCourante, int isGet)
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
	tabClients->nbClients++;
}
