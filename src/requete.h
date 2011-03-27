#if ! defined ( REQUETE_H_ )
#define REQUETE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>

#include "utils.h"


int parseInt(char* entier);

void initReq(struct requete* req);

void traiteRequete(struct requete* req, struct videoClient* videoClient, int epollfd, int sock);

void traiteChaine(char* chaine, struct requete* req, struct videoClient* videoClient, int epollfd, int sock);

#endif // REQUETE_H_
