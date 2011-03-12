#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h> //Pour STDIN_FILENO

#include "utils.c"

#define MAX_EVENTS 10
#define MAX_HEADER 200
#define MAX_STR 32


char * build_date()
{
  return "00 / 00 / 00";
}

char * build_http_header(char * type, int size)
{
  char * header = malloc(MAX_HEADER * sizeof(char));
  memset(header, '\0', MAX_HEADER);
  strcat(header, "HTTP/1.1 200 OK\nDate: ");
  
  /*Insertion de la date actuelle*/
  strcat(header, build_date());

  strcat(header, "\nServer: ServLib (Unix) (Arch/Linux)\nAccept-Ranges: bytes\n\
Content-Length: ");

  /*Insertion de la taille*/
  char tmp[MAX_STR] = {'\0'};
  sprintf(tmp, "%d", size);
  strcat(header, tmp);

  strcat(header, "\nConnection: close\nContent-Type: ");

  /*Insertion du type*/
  strcat(header, type);

  strcat(header, "; charset=UTF-8\n\n");

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
  char * buf = NULL;
  /*On recuper le fichier sous forme de chaine de cara*/
  file_to_buffer(&buf, &size);
  /*On construit l'entete HTML aproprié*/
  char * header = build_http_header("text/plain", size);

  /*Et on envoie les données*/
  puts("Going to send");
  send(fd, header, strlen(header), MSG_MORE);
  printf("send : %s\n", strerror(errno));
  send(fd, buf, size, 0);
  printf("send : %s\n", strerror(errno));

  /*On libere les ressources alloué*/
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

  struct stockClient {
	  int nbClients;
	  struct requete * requetes;
	  };

  struct stockClient tabClient;
  tabClient.requetes = (struct requete *)malloc(10*sizeof(struct requete));
  tabClient.nbClients = 0;

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
			csock = createSockClientEvent(epollfd, sock);	    
        }
	   	else if (events[n].data.fd == socktest)
	   	{

			csocktest = createSockClientEvent(epollfd, socktest);
			tabClient.nbClients++;
			tabClient.requetes[tabClient.nbClients-1].sock = csocktest;
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
            printf("%d\n", events[n].events);
            if (events[n].data.fd == csock)
			{
				if (events[n].events == 5)
                send_get_answer(events[n].data.fd);
			}
            if (events[n].data.fd == csocktest)
			{
				if (events[n].events == 5)
				{					
					printf("%s\n", "buffer");
					char buffer[512];
					printf("%s\n", "recv");
					recv(csocktest, buffer, 512*sizeof(char),0);
					printf("%s\n", "traite");
					traiteChaine(buffer, &tabClient.requetes[1]);
					printf("%s\n", "over");
				}
			}
        }
    }
} //Fin de la boucle principale
  
  //TODO: fermer les ressources proprement ici
  
}  

int main(int argc, char ** argv)
{
  central(8081);
  return 0;
}
