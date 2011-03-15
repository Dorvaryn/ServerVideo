#include "cata.h"

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

	int l3 = strlen(buff);
	for(i=0;i<l3;i++)
	{
		buff[i] = '\0';
	}
	int l = strlen(temp);
	for(i=0;i<l;i++)
	{
		temp[i] = '\0';
	}
	
	strcat(buff,"ServerAddress: 127.0.0.1\r\n");
	strcat(buff,"ServerPort: 8081\r\n");

	fgets(temp,512,f);
	printf("fgets : %s\n", strerror(errno));
	do
	{
		strcpy(temp2,DATA_DIRECTORY);
		strncat(temp2,temp,strlen(temp)-2);

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
	strcat(buff,"\r\n");
	printf("%s\n", "done");
	fclose(f);
	return buff;
}
