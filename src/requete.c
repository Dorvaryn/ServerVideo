#include "requete.h"
#include "envoi.h"

/**
 * Converti un char* en un entier.
 * Le char* doit représenter un nombre entre -1 et la taille max
 * d'un int non signé.
 */
int parseInt(char* entier) {
    if(strcmp(entier, "-1") == 0) {
        return -1;
    }
    
    int res = 0;
    int i;
    for(i = 0; entier[i] != '\0'; i++) {
        char c = entier[i];
        if(isdigit(c)) {
            res = res*10+(c-'0');
        } else {
            return PARSE_ERROR;
        }
    }

    return res;
}

void initReq(struct requete* req) {
    req->type = NON_DEFINI;
    
    req->isOver = 0;
    
    req->imgId = -2;
    req->listenPort = -1;
    req->fragmentSize = -1;
    
    req->inWord = 0;
    req->space = 0;
    req->crlfCounter = 0;
    
    req->mot = 0;
    req->motPosition = 0;
    
    req->reqPosition = 0;
}

void traiteRequete(struct requete* req, int clientSocket) {
    switch(req->type) {
        case BAD_REQUEST:
            puts("E: Mauvaise requete");
            break;
        case GET:
            if(req->fragmentSize != -1) 
			{
                printf("GET id:%d port:%d frag_size:%d\n", req->imgId, req->listenPort, req->fragmentSize);
            } else if(req->listenPort != -1) 
			{
                printf("GET id:%d port:%d\n", req->imgId, req->listenPort);
                //TODO: Se connecter au client en TCP sur le port listenPort et mémoriser ce port dans un structure...
                
            } else {
                printf("GET id:%d\n", req->imgId);
                
                struct envoiTcp* envoi = malloc(sizeof(struct envoiTcp));
                
                envoi->state = NOTHING_SENT;
                envoi->clientSocket = clientSocket;
                
                //TODO: ligne suivante : traiter avec le id et les fichiers du catalogue correspondant au port
                envoi->curFile = fopen("./Images/img1.bmp", "r");
                
                send(envoi);
            }
            break;
        case START:
            printf("START\n");
            break;
        case PAUSE:
            printf("PAUSE\n");
            break;
        case END:
            printf("END\n");
            break;
        case ALIVE:
            printf("ALIVE id:%d port:%d\n", req->imgId, req->listenPort);
            break;
        default:
            break;
    }
}

void traiteChaine(char* chaine, struct requete* req, int clientSocket) {

    if(req->mot == 0) {
        req->mot = malloc(MAX_TOCKEN*sizeof(char));
    }
    
    if(req->type != NON_DEFINI) return;

    int i;
    for(i=0; chaine[i] != '\0' && !req->isOver; i++) {
        
        char c = chaine[i];
		//printf("%s\n",chaine);

        //est-ce que le caractère est un espace ?
        req->space = (c == ' ' || c == '\n' || c == '\r');
        
        if(req->inWord && req->space) { //Le mot est fini, on peut le traiter
            req->mot[req->motPosition] = '\0';
            req->inWord = 0;
            
			//printf("%s\n",req->mot);

            //Traitement du mot lu
            //puts(req->mot);
            if(req->reqPosition == 0) {
                //puts("choix..");
                if(strcmp(req->mot, "GET") == 0) {
                    req->type = GET;
                } else if(strcmp(req->mot, "START") == 0) {
                    req->type = START;
                } else if(strcmp(req->mot, "PAUSE") == 0) {
                    req->type = PAUSE;
                } else if(strcmp(req->mot, "END") == 0) {
                    req->type = END;
                } else if(strcmp(req->mot, "ALIVE") == 0) {
                    req->type = ALIVE;
                } else {
                    req->type = BAD_REQUEST;
                }
            } else if(req->reqPosition == 1) {
                int numero = parseInt(req->mot);
                if(numero == PARSE_ERROR) {
                    req->imgId = 0;
                } else {
                    req->imgId = numero;
                }
            } else if(req->reqPosition == 2 && strcmp(req->mot, "LISTEN_PORT") != 0) {
                req->type = BAD_REQUEST;
            } else if(req->reqPosition == 3) {
                int numero = parseInt(req->mot);
                if(numero == PARSE_ERROR) {
                    req->type = BAD_REQUEST;
                } else {
                    req->listenPort = numero;
                }
            } else if(req->reqPosition == 4) {
                if(req->type == ALIVE || strcmp(req->mot, "FRAGMENT_SIZE") != 0) {
                    req->type = BAD_REQUEST;
                }
            } else if(req->reqPosition == 5) {
                int numero = parseInt(req->mot);
                if(numero == PARSE_ERROR) {
                    req->type = BAD_REQUEST;
                } else {
                    req->fragmentSize = numero;
                }
            } else if(req->reqPosition == 6) {
                req->type = BAD_REQUEST;
            }
            
            req->reqPosition++;
            
            /*if(c=='\r') {
                req->crlfCounter = 1;
            }*/
            
        } else if(req->inWord && !req->space) { //Le mot continue
            req->mot[req->motPosition] = c;
            req->motPosition++;
        } else if(!req->inWord && !req->space) { //Le mot commence
            req->crlfCounter = 0;
            req->motPosition = 0;
            req->inWord = 1;
            req->mot[req->motPosition] = c;
            req->motPosition++;
        } else { //Sinon les espaces continuent
            if(c == '\n') {
                req->crlfCounter++;
            }
            if(req->crlfCounter == 2) {
                req->isOver = 1;
                //puts(req->mot);
            }
        }
        
        //printf("### %c %d %d %d ###\n", c, req->inWord, req->space, req->motPosition);
    }
    
    if(req->isOver) {
        free(req->mot);
        
        if(req->type == GET && req->imgId == -2) {
            req->type = BAD_REQUEST;
        }
        if(req->type == ALIVE && req->imgId == -2) {
            req->type = BAD_REQUEST;
        }
        /*puts("requete terminée");
        if(req->type == BAD_REQUEST) {
            puts("mauvaise requete");
        } else*/
        traiteRequete(req, clientSocket);
        initReq(req);
    }
}

/*int main() {
    struct requete req;
    initReq(&req);

    // tests
    //assert(parseInt("1") == 1);
    //assert(parseInt("-1") == -1);
    //assert(parseInt("e") == PARSE_ERROR);
    //assert(parseInt("523654") == 523654);
    
    
    //req.mot = malloc(MAX_TOCKEN*sizeof(char));
    
    traiteChaine("ALIV", &req);
    traiteChaine("E 0 LISTEN_PORT \n  5\r\n \r\n", &req);
    
    traiteChaine("START\r\n\r\n", &req);
    
    traiteChaine("PAUSE\r\n\r\n", &req);
    
    traiteChaine("END\r\n\r\n", &req);
    
    traiteChaine("GET\r\n\r\n", &req);
    
    traiteChaine("GET -1\r\n\r\n", &req);
    
    traiteChaine("GET 5\r\n LISTEN_PORT 404\r\n\r\n", &req);
    
    traiteChaine("GET 1024\r\n LISTEN_PORT 4096\r\n FRAGMENT_SIZE 32 \r\n\r\n", &req);
    
    //printf("### %d ###\n", req.type);
    
    return 0;
}*/
