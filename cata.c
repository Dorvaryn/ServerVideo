#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h> //Pour STDIN_FILENO


char * buildCatalogue (char * file)
{
	/*Ouverture du fichier*/
	FILE * f = fopen(file, "r");
	printf("fopen : %s\n", strerror(errno));

	int i;
	char * buff = (char *)malloc(999*sizeof(char));
	char * temp = (char *)malloc(512*sizeof(char));
	char * temp2 = (char *)malloc(512*sizeof(char));

	fgets(temp,512,f);
	fgets(temp,512,f);
	
	int l = strlen(temp);
	for(i=0;i<l;i++)
	{
		   temp[i] = '\0';
	}

	fgets(temp,512,f);
	printf("fgets : %s\n", strerror(errno));
	do
	{

		strncpy(temp2,temp,strlen(temp)-2);

		printf("%s\n", temp2);
		
		FILE * g = fopen(temp2,"r");
		printf("fopen : %s\n", strerror(errno));
		strcat(buff,"Object ");

		int l = strlen(temp2);
		for(i=0;i<l;i++)
		{
			   temp2[i] = '\0';
		}
		int l2 = strlen(temp);
		for(i=0;i<l2;i++)
		{
			   temp[i] = '\0';
		}

		int j;
		for(j = 0; j < 7; j++)
		{
			char * tmp = (char *)malloc(512*sizeof(char));
			char  * tmp2 = (char *)malloc(512*sizeof(char));

			fgets(tmp,512,g);
			printf("fgets : %s\n", strerror(errno));
			strncpy(tmp2,tmp,strlen(tmp)-2);
			strcat(buff,tmp2);
			strcat(buff," ");

			int l2 = strlen(tmp2);
			for(i=0;i<l2;i++)
			{
				   tmp2[i] = '\0';
			}
			int l = strlen(tmp);
			for(i=0;i<l;i++)
			{
				   tmp[i] = '\0';
			}
		}
		printf("%s\n", "fin");
		strcat(buff,"\r\n");
		printf("%s\n", "strcat");
		fclose(g);
		fgets(temp,512,f);
		printf("fgets : %s\n", strerror(errno));

	}while(!feof(f));
	strcat(buff,"\0");
	printf("%s\n", "done");
	fclose(f);
	return buff;
}
