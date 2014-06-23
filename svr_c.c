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
#include <limits.h>
#include <netdb.h>
#include <pthread.h>
#include "list.h"
#define NUMTRIES 99
#define EVENTSIZE 50
#define MSGSIZE 80

pthread_mutex_t mutexList;
int execute;

struct sendThreadArgs {

   char *remotePort;
   char *domain;
   struct list *eventList;
   int localPort;
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

char *createMsg(char *eventMsg) {

   time_t rawtime = time(NULL);
   struct tm *timeEvent = localtime(&rawtime);
   char *strTime = asctime(timeEvent);
   strTime[strlen(strTime)-1] = '\0';

   char *msg = calloc(MSGSIZE,sizeof(char));
   memset(msg,' ',MSGSIZE*sizeof(char));

   sprintf(msg, "%s, %s",strTime,eventMsg);

   int lengthEvent = strlen(msg);
   msg[lengthEvent] = ' ';
   if (msg[lengthEvent-1] == '\n')
      msg[lengthEvent-1] = ' ';
   msg[MSGSIZE-1] = '\0';
   printf("Message to send: %s\n", msg);

   return msg;

}

void connectionServer(int *socketfd, struct sockaddr addr, struct list *eventList) {

   int numTries;

   for (numTries = NUMTRIES; numTries > -1; --numTries) {

      /* Intenta establecer una conexion al servidor */
      if((connect(*socketfd,&addr, sizeof(addr)) < 0) && numTries > 0){
         printf("Connection to server failed. Trying again...\n");
         sleep(1);
      } else
         break;

      if (numTries % 10 == 0) {

         char *msg = createMsg("communication offline");

         pthread_mutex_lock(&mutexList);
         addElement(msg,eventList);
         pthread_mutex_unlock(&mutexList);

         printf("Connection to server failed. Trying again in 30 seconds.\n");
         sleep(30);
      }
   }

   if (numTries == -1) {
      printf("Connection to server failed.\n");

      //TODO SEND EMAIL

      exit(EXIT_FAILURE);
   }

   printf("Connection established.\n");
}

void createSocket(char *remotePort, int localPort, int *sockfd, char *domain, struct sockaddr *addr) {


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

   if (localPort != -1) {

      struct sockaddr_in localAddr;

      localAddr.sin_family = AF_INET;
      localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
      localAddr.sin_port = htons(localPort);

      bind(*sockfd, (struct sockaddr *)&localAddr, sizeof(localAddr));

      if (errno == EADDRINUSE) {
         printf("Error: port #%d is already in use.\n",localPort);
         exit(EXIT_FAILURE);
      }
   }

   free(servInfo);

}


void reconnect(int *socketfd, char *remotePort, int localPort, char *domain, char *event, struct list *eventList) {

   close(*socketfd);

   struct sockaddr addr;
   printf("Server was down. Reconnecting...\n");

   createSocket(remotePort,localPort,socketfd,domain,&addr);
   connectionServer(socketfd,addr,eventList);

   pthread_mutex_lock(&mutexList);
   restoreBackup(eventList);
   addElement(event,eventList);
   pthread_mutex_unlock(&mutexList);

}

void sendEventsHelper(char *remotePort, int localPort, char *domain, int *socketfd, struct list *eventList) {

   char *event;

   while (listSize(eventList) > 0) {

      pthread_mutex_lock(&mutexList);
      event = getElement(eventList);
      pthread_mutex_unlock(&mutexList);

      errno = 0;
      int numBytes = send(*socketfd,event,strlen(event),MSG_NOSIGNAL);
      int numTries = 0;


      if (errno == EPIPE) { // para cuando hay un broken pipe, reestablecer la conexion

         reconnect(socketfd,remotePort,localPort,domain,event,eventList);

      } else if (numBytes == -1) {

         while (numBytes < 0 && numTries < 10) {

            printf("Error sending message. Trying again...\n");

            errno = 0;
            numBytes = send(*socketfd,event,strlen(event),MSG_NOSIGNAL);

            ++numTries;
            sleep(1);
         }

         if (numTries == 10) {

            reconnect(socketfd,remotePort,localPort,domain,event,eventList);

         }
      }
   }
}


void *sendEvents(void *sendArgs) {

   struct sendThreadArgs *args = (struct sendThreadArgs *) sendArgs;

   char *remotePort = args->remotePort;
   char *domain = args->domain;
   struct list *eventList = args->eventList;
   int localPort = args->localPort;

   int socketfd;
   struct sockaddr addr;

   createSocket(remotePort,localPort,&socketfd,domain,&addr);
   connectionServer(&socketfd,addr,eventList);

   while (execute) {

      while (listSize(eventList) == 0 && execute)
         sleep(2);

      sendEventsHelper(remotePort,localPort,domain,&socketfd,eventList);
   }

   if (listSize(eventList) > 0)
      sendEventsHelper(remotePort,localPort,domain,&socketfd,eventList);

   pthread_exit(NULL);
}



void *readEvents(void *eList) {

   struct list *eventList = (struct list *) eList;
   int lenEvent = EVENTSIZE;
   char *eventMsg = calloc(EVENTSIZE,sizeof(char));

   while (1) {

      if (fgets(eventMsg,lenEvent,stdin) == NULL) {
         execute = 0;
         break;
      }

      char *msg = createMsg(eventMsg);

      pthread_mutex_lock(&mutexList);
      addElement(msg,eventList);
      pthread_mutex_unlock(&mutexList);

   }

   free(eventMsg);
}



int portStrToNum(char *portStr) {

   errno = 0;
   char *endptr;

   if (strcmp(portStr,"") == 0)
      return -1;

   int port = strtol(portStr,&endptr,10);

   if ((errno == ERANGE || (errno != 0 && port == 0)) || (endptr == portStr) ||
      (*endptr != '\0') || (port < 1) || (port > USHRT_MAX)) {

      printf("Invalid port number.\n");
      exit(EXIT_FAILURE);
   }

   return port;
}

void intializeClient(pthread_t *readThread, pthread_t *sendThread,struct sendThreadArgs sendArgs, struct list *eventList) {

   execute = 1;

   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);

   pthread_mutex_init(&mutexList,NULL);

   if (pthread_create(readThread,NULL,readEvents,(void *) eventList)) {

      printf("Error creating read thread.\n");
      exit(EXIT_FAILURE);
   }

   if (pthread_create(sendThread,&attr,sendEvents,(void *) &sendArgs)) {

      printf("Error creating send thread.\n");
      exit(EXIT_FAILURE);
   }

   pthread_attr_destroy(&attr);
}

int main(int argc, char *argv[]) {

   char *domain = calloc(100,sizeof(char));
   char *remotePort = calloc(10,sizeof(char));
   char *localPort = calloc(10,sizeof(char));

   clientArguments(argc,argv,domain,remotePort,localPort);

   int localPortNum = portStrToNum(localPort);

   struct list eventList;
   initializeList(&eventList);

   char *msg = createMsg("application started");
   addElement(msg,&eventList);

   void *status;

   struct sendThreadArgs sendArgs;

   sendArgs.remotePort = remotePort;
   sendArgs.domain = domain;
   sendArgs.eventList = &eventList;
   sendArgs.localPort = localPortNum;

   pthread_t *readThread = calloc(1,sizeof(pthread_t));
   pthread_t *sendThread = calloc(1,sizeof(pthread_t));

   intializeClient(readThread,sendThread,sendArgs,&eventList);

   while (execute)
      sleep(2);

   if (pthread_join(*sendThread,&status))
      printf("Error joining thread.\n");

   free(readThread);
   free(sendThread);
   free(domain);
   free(remotePort);
   free(localPort);
   destroyList(&eventList);

   return 0;
}
