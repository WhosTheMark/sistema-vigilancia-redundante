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

   for (numTries = 99; numTries > -1; --numTries) {

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

int sendEvents(int *socketfd, struct list *eventList) {

   char *event;

   while (listSize(eventList) > 0) {

      event = getElement(eventList);
      errno = 0;
      int numBytes = send(*socketfd,event,strlen(event),MSG_NOSIGNAL);
      int numTries = 0;

      if (errno == EPIPE) { // para cuando hay un broken pipe, reestablecer la conexion

         printf("event: %s\n",event);
         addElement(event,eventList);
         return 0;
      }

      if (numBytes == -1) {


         while (numBytes < 0 && numTries < 10) {

            printf("Error sending message. Trying again...\n");

            errno = 0;
            numBytes = send(*socketfd,event,strlen(event),MSG_NOSIGNAL);

            ++numTries;
            sleep(1);
         }

         if (numTries == 10) {
            addElement(event,eventList);

         }
      }
   }

   return 1;
}

int readEvents(FILE *fd, int socketfd, struct list *eventList) {

   int numMsgs;

   int endFile = fscanf(fd, "%d", &numMsgs) == EOF;
   int successSending = 0;

   while (!endFile && numMsgs != 0) {

      int eventCode;
      char *eventMsg;

      printf("Sending messages\n");

      while (!endFile && numMsgs != 0) {

         endFile = fscanf(fd, "%d", &eventCode) == EOF;

         time_t rawtime = time(NULL);
         struct tm *timeEvent = localtime(&rawtime);

         eventMsg = calloc(40,sizeof(char));
         sprintf(eventMsg, "%2d,%s", eventCode, asctime(timeEvent));
         addElement(eventMsg,eventList);

         --numMsgs;
      }

      successSending = sendEvents(&socketfd,eventList);

      if (!successSending) {
         close(socketfd);
         return 0;
      }

      endFile = fscanf(fd, "%d", &numMsgs) == EOF;
      sleep(10);
   }

   int numTries;

   for (numTries = 0; numTries < 20 && !successSending; ++numTries) {
      successSending = sendEvents(&socketfd,eventList);
   }

   if (!successSending) {
      close(socketfd);
      return 0;
   }

   fclose(fd);

   if (endFile) {
      printf("Format of file is incorrect.\n");
      exit(EXIT_FAILURE);
   }

   return 1;
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

   char *path = calloc(100,sizeof(char));
   path = "tests/msg1.txt";
   FILE *fd = fopen(path,"r");

   if (fd == NULL) {
      printf("File %s cannot be opened.", path);
      perror("");
      exit(EXIT_FAILURE);
   }

   char *domain = calloc(100,sizeof(char));
   char *remotePort = calloc(10,sizeof(char));
   char *localPort = calloc(10,sizeof(char));

   clientArguments(argc,argv,domain,remotePort,localPort);

   int socketfd = 0;

   int remotePortNum, localPortNum;
   struct sockaddr addr;

   struct list eventList;
   initializeList(&eventList);

   int success = 0;

   while (!success) {

      createSocket(remotePort,&socketfd,domain,&addr);

      connectionServer(&socketfd,addr);

      success = readEvents(fd,socketfd,&eventList);

   }

   close(socketfd);

   return 0;
}
