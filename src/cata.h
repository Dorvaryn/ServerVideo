#if ! defined ( CATA_H_ )
#define CATA_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h> //Pour STDIN_FILENO

#define DATA_DIRECTORY "data/"

char * buildCatalogue (char * file);

#endif // CATA_H_
