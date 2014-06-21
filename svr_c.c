#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "list.h"
#include <limits.h>
#include <netdb.h>
#define NUMTRIES 99

pthread_mutex_t mutexList;

struct sendThreadArgs {

   //NOTE Local port?
   char *remotePort;
   char *domain;
   struct list *eventList;
};

void usage() {

   printf("Usage: ./svr_c -d <nombre_modulo_central> -p <puerto_svr_s> [-l <puerto_local>]\n");
   exit(EXIT_FAILURE);
}

void clientArguments(int argc, char *argv[], char *domain, char *remotePort, char *localPort) {

   int flagD = 0, flagP = 0, flagL = 0;

   while ((argc > 1) && (argv[1][0] == '-')) {

      switch (argv[1][1]) {

         case 'd':

            if (argc > 2)
               strcpy(domain,argv[2]);
            else {
               printf("The flag -d requires an argument.\n");
               usage();
            }
            ++flagD;
            break;

         case 'p':

            if (argc > 2)
               strcpy(remotePort,argv[2]);
            else {
               printf("The flag -p requires an argument.\n");
               usage();
            }
            ++flagP;
            break;

         case 'l':

            if (argc > 2)
               strcpy(localPort,argv[2]);
            else {
               printf("The flag -l requires an argument.\n");
               usage();
            }
            ++flagL;
            break;

         default:

            printf("Incorrect arguments for initiating client.\n");
            usage();
      }
      argv += 2;
      argc -= 2;
   }

   if (flagD != 1 || flagP != 1 || flagL > 1) {
      printf("Incorrect arguments for initiating client.\n");
      usage();
   }
}

void connectionServer(int *socketfd, struct sockaddr addr) {

   int numTries;

   for (numTries = NUMTRIES; numTries > -1; --numTries) {

      /* Intenta establecer una conexion al servidor */
      if((connect(*socketfd,&addr, sizeof(addr)) < 0) && numTries > 0){
         printf("Connection to server failed. Trying again...\n");
         sleep(1);
      } else
         break;

      if (numTries % 10 == 0) {
         printf("Connection to server failed. Trying again in 30 seconds.\n");
         sleep(30);
      }
   }

   if (numTries == -1) {
      printf("Connection to server failed.\n");
      exit(EXIT_FAILURE);
   }
}

void createSocket(char *remotePort, int *sockfd, char *domain, struct sockaddr *addr) {

   //NOTE agregar archivo para eventos del ATM

   struct addrinfo hints, *servInfo, *p;

   memset(&hints,0,sizeof(hints));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   if (getaddrinfo(domain,remotePort,&hints,&servInfo) != 0) {
      printf("%s is not a valid domain.\n",domain);
      exit(EXIT_FAILURE);
   }

   if ((*sockfd = socket(servInfo->ai_family, servInfo->ai_socktype,
      servInfo->ai_protocol)) == -1) {

      printf("Error: could not create socket.\n");
      exit(EXIT_FAILURE);
   }

   *addr = *(servInfo->ai_addr);
}

int sendEvents(void *sendArgs) {

   struct sendThreadArgs *args = (struct sendThreadArgs *) sendArgs;

   char *remotePort = (char *) args->remotePort;
   char *domain = (char *) args->domain;
   struct list *eventList = (struct list *) args->eventList;

   int socketfd;
   struct sockaddr addr;

   createSocket(remotePort,&socketfd,domain,&addr);
   connectionServer(&socketfd,addr);


   while (1) {

      while (listSize(eventList) == 0);

      char *event;

      while (listSize(eventList) > 0) {

         // SEMAFORO

         pthread_mutex_lock(&mutexList);
         event = getElement(eventList);
         pthread_mutex_unlock(&mutexList);

         errno = 0;
         int numBytes = send(socketfd,event,strlen(event),MSG_NOSIGNAL);
         int numTries = 0;

         if (errno == EPIPE) { // para cuando hay un broken pipe, reestablecer la conexion

            close(socketfd);

            createSocket(remotePort,&socketfd,domain,&addr);
            connectionServer(&socketfd,addr);

            printf("event: %s\n",event);
            pthread_mutex_lock(&mutexList);
            addElement(event,eventList);
            pthread_mutex_unlock(&mutexList);
         }

         if (numBytes == -1) {

            while (numBytes < 0 && numTries < 10) {

               printf("Error sending message. Trying again...\n");

               errno = 0;
               numBytes = send(socketfd,event,strlen(event),MSG_NOSIGNAL);

               ++numTries;
               sleep(1);
            }

            if (numTries == 10) {

               close(socketfd);

               createSocket(remotePort,&socketfd,domain,&addr);
               connectionServer(&socketfd,addr);


               pthread_mutex_lock(&mutexList);
               addElement(event,eventList);
               pthread_mutex_unlock(&mutexList);

            }
         }
      }
   }

   return 1;
}

void *readEvents(void *eList) {

   struct list *eventList = (struct list *) eList;
   int lenEvent = 28;

   while (1) {

      char *eventMsg = calloc(40,sizeof(char));
      memset(eventMsg,' ',40*sizeof(char));
      //fscanf(stdin, "%s\n", eventMsg);

      fgets(eventMsg,lenEvent,stdin);

      int lengthEvent = strlen(eventMsg);
      eventMsg[lengthEvent] = ' ';
      eventMsg[lengthEvent-1] = ' ';
      eventMsg[28] = '\0';
      printf("event: %s, size: %d\n", eventMsg, lengthEvent);

      time_t rawtime = time(NULL);
      struct tm *timeEvent = localtime(&rawtime);

      //sprintf(eventMsg, "%2d,%s", eventCode, asctime(timeEvent));
      pthread_mutex_lock(&mutexList);
      addElement(eventMsg,eventList);
      pthread_mutex_unlock(&mutexList);


   }
}



int portStrToNum(char *portStr) {

   errno = 0;
   char *endptr;
   int port = strtol(portStr,&endptr,10);

   if ((errno == ERANGE || (errno != 0 && port == 0)) || (endptr == portStr) ||
      (*endptr != '\0') || (port < 1) || (port > USHRT_MAX)) {

      printf("Invalid port number.\n");
      exit(EXIT_FAILURE);
   }

   return port;
}


int main(int argc, char *argv[]) {

   char *domain = calloc(100,sizeof(char));
   char *remotePort = calloc(10,sizeof(char));
   char *localPort = calloc(10,sizeof(char));

   clientArguments(argc,argv,domain,remotePort,localPort);

   struct list eventList;
   initializeList(&eventList);

   int success = 0;
   void *status;

   pthread_mutex_init(&mutexList,NULL);

   pthread_t *readThread = calloc(1,sizeof(pthread_t));
   pthread_t *sendThread = calloc(1,sizeof(pthread_t));

   if (pthread_create(readThread,NULL,readEvents,(void *) &eventList)) {

      printf("Error creating read thread.\n");
      exit(EXIT_FAILURE);
   }

   struct sendThreadArgs sendArgs;

   sendArgs.remotePort = remotePort;
   sendArgs.domain = domain;
   sendArgs.eventList = &eventList;

   if (pthread_create(sendThread,NULL,sendEvents,(void *) &sendArgs)) {

      printf("Error creating send thread.\n");
      exit(EXIT_FAILURE);
   }

   while (1);



   return 0;
}
