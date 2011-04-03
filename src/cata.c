#include "cata.h"

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

char * buildCatalogue (int epollfd, struct tabFlux * tabFluxTCP, struct tabFlux * tabFluxUDP, struct tabFlux * tabFluxMCAST)
{

	char * buff = (char *)malloc(MAX_CATA*sizeof(char));
	memset(buff, '\0', MAX_CATA);

	/*Ouverture du fichier*/
	FILE * f = fopen("data/catalogue.txt", "r");

	int baseFluxTCPCourante = BASE_FICHIERS;
	int baseFluxUDPCourante = BASE_FICHIERS;
	int baseFluxMCASTCourante = BASE_FICHIERS;

	char * temp = (char *)malloc(512*sizeof(char));
	char * temp2 = (char *)malloc(512*sizeof(char));
	memset(temp, '\0', 512);
	memset(temp2, '\0', 512);
	
	fgets(temp,512,f);
	strcat(buff,temp);

	fgets(temp,512,f);
	strcat(buff,temp);
						
	int serverPort;
	sscanf(temp,"%*12s%d",&serverPort);
	createFichier(epollfd, tabFluxTCP, serverPort, &baseFluxTCPCourante, 0);

	//strcat(buff,"ServerAddress: 10.0.2.2\r\n");
	//strcat(buff,"ServerAddress: 127.0.0.1\r\n");
	//strcat(buff,"ServerPort: 8081\r\n");

	fgets(temp,512,f);
	do
	{
		strcpy(temp2,DATA_DIRECTORY);
		strncat(temp2,temp,strlen(temp)-2);

		FILE * g = fopen(temp2,"r");
		strcat(buff,"Object ");
		
		memset(temp, '\0', 512);
		memset(temp2, '\0', 512);

		int j = 0;
		char * tmp = (char *)malloc(512*sizeof(char));
		memset(tmp, 0, 512);
		char  * tmp2 = (char *)malloc(512*sizeof(char));
		memset(tmp2, 0, 512);

		fgets(tmp,512,g);
		
		int port;
		char * adresse = (char *)malloc(512*sizeof(char));
		memset(adresse, '\0', 512*sizeof(char));

		int typeCourant;
		while (!feof(g))
		{
			strncpy(tmp2,tmp,strlen(tmp)-2);
			
			if (j < 7)
			{
				if (j == 3)
				{
						sscanf(tmp2,"%*8s%s",adresse);
				}
				else if (j == 4)
				{
						sscanf(tmp2,"%*5s%d",&port);
				}
				else if (j == 5)
				{	
					char protocole[512];
					sscanf(tmp2,"%*9s%s",protocole);
					if(strcmp(protocole,"TCP_PULL") == 0)
					{
						typeCourant = 0;
						createFichier(epollfd, tabFluxTCP, port, &baseFluxTCPCourante, TCP_PULL);
						tabFluxTCP->flux[tabFluxTCP->nbFlux-1].infosVideo.type = TCP_PULL;
					}
					else if(strcmp(protocole,"TCP_PUSH") == 0)
					{
						typeCourant = 0;
						createFichier(epollfd, tabFluxTCP, port, &baseFluxTCPCourante, TCP_PUSH);
						tabFluxTCP->flux[tabFluxTCP->nbFlux-1].infosVideo.type = TCP_PUSH;
					}
					else if(strcmp(protocole,"UDP_PULL") == 0)
					{
						typeCourant = 1;
						createFichier(epollfd, tabFluxUDP, port, &baseFluxUDPCourante, UDP_PULL);
						tabFluxUDP->flux[tabFluxUDP->nbFlux-1].infosVideo.type = UDP_PULL;
					}
					else if(strcmp(protocole,"UDP_PUSH") == 0)
					{
						typeCourant = 1;
						createFichier(epollfd, tabFluxUDP, port, &baseFluxUDPCourante, UDP_PUSH);
						tabFluxUDP->flux[tabFluxUDP->nbFlux-1].infosVideo.type = UDP_PUSH;
					}
					else if(strcmp(protocole,"MCAST_PUSH") == 0)
					{
						typeCourant = 2;
						createFichier(epollfd, tabFluxMCAST, port, &baseFluxMCASTCourante, MCAST_PUSH);
						tabFluxMCAST->flux[tabFluxMCAST->nbFlux-1].infosVideo.type = MCAST_PUSH;
						
						tabFluxMCAST->flux[tabFluxMCAST->nbFlux-1].adresse = (char *)malloc(512*sizeof(char));
						memset(tabFluxMCAST->flux[tabFluxMCAST->nbFlux-1].adresse, '\0', 512*sizeof(char));
						strcpy(tabFluxMCAST->flux[tabFluxMCAST->nbFlux-1].adresse,adresse);
					}
				}	
				else if (j == 6)
				{
					char fps[512];
					sscanf(tmp2,"%*4s%s",fps);
					if(typeCourant == 0)
					{
						tabFluxTCP->flux[tabFluxTCP->nbFlux-1].infosVideo.fps = atof(fps);
					}
					else if(typeCourant == 1)
					{
						tabFluxUDP->flux[tabFluxUDP->nbFlux-1].infosVideo.fps = atof(fps);
					}
					else
					{
						tabFluxMCAST->flux[tabFluxMCAST->nbFlux-1].infosVideo.fps = atof(fps);
					}
				}
				strcat(buff,tmp2);
				strcat(buff," ");

			}
			else
			{
				char image[512];
				sprintf(image, "%s%s", "./data/",tmp2);
				if(typeCourant == 0)
				{
					addImage(image, &tabFluxTCP->flux[tabFluxTCP->nbFlux-1].infosVideo);
				}
				else if(typeCourant == 1)
				{
					addImage(image, &tabFluxUDP->flux[tabFluxUDP->nbFlux-1].infosVideo);
				}
				else
				{
					addImage(image, &tabFluxMCAST->flux[tabFluxMCAST->nbFlux-1].infosVideo);
				}
			}

			memset(tmp, '\0', 512);
			memset(tmp2, '\0', 512);
			fgets(tmp,512,g);
			j++;
		}
		strcat(buff,"\r\n");
		fclose(g);
		fgets(temp,512,f);
		
		free(tmp);
		tmp = NULL;
        free(tmp2);
        tmp2 = NULL;
        free(adresse);
        adresse = NULL;

	}while(!feof(f));
	strcat(buff,"\r\n");
	fclose(f);

	char * header;
	/*On construit l'entete HTML aproprié*/
	header = build_http_header("text/plain", strlen(buff));

	char * buff2 = (char *)malloc((MAX_HEADER + MAX_CATA)*sizeof(char));
	memset(buff2, '\0', (MAX_HEADER + MAX_CATA));
	strcpy(buff2, header);
	strcat(buff2, buff);

    free(temp);
    temp = NULL;
    free(temp2);
    temp2 = NULL;
	free(buff);
	buff = NULL;
	free(header);
	header = NULL;

	return buff2;
}
