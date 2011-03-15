#include "cata.h"
#include "utils.h"

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
	FILE * f = fopen("data/catalogue.txt", "r");
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
	buf = buildCatalogue("data/catalogue.txt");
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

void connectClient(int epollfd, struct tabClients * tabClients, int sock, int baseCourante, int isGet)
{
	if (tabClients->nbClients >= baseCourante)
	{
		baseCourante *=2;
		struct sockClient * temp;
		temp = (struct sockClient *) realloc(tabClients->clients,
				baseCourante*sizeof(struct sockClient));
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
	initReq(&tabClients->clients[tabClients->nbClients].requete);
	tabClients->clients[tabClients->nbClients].isGET = isGet;
	tabClients->nbClients++;
}
