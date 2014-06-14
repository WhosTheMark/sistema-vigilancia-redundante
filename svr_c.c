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
#define PUERTO 6666


void sendEvents(int socketfd, struct list *eventList) {

   char *event;

   while (listSize(eventList) > 0) {

      event = getElement(eventList);
      errno = 0;
      int numBytes = send(socketfd,event,strlen(event),MSG_NOSIGNAL);
      int numTries = 0;

      if (numBytes == -1) {

         //TODO errno = EPIPE para cuando hay un broken pipe, reestablecer la conexion

         while (numBytes < 0 && numTries < 10) {

            printf("Error sending message. Trying again...\n");

            errno = 0;
            numBytes = send(socketfd,event,strlen(event),MSG_NOSIGNAL);

            ++numTries;
            sleep(1);
         }
      }
   }
}

void readEvents(FILE *fd, int socketfd, struct list *eventList) {

   int numMsgs;

   int endFile = fscanf(fd, "%d", &numMsgs) == EOF;

   while (!endFile && numMsgs != 0) {

      int eventCode;
      char *eventMsg;

      printf("Sending messages\n");

      while (!endFile && numMsgs != 0) {

         endFile = fscanf(fd, "%d", &eventCode) == EOF;

         time_t rawtime = time(NULL);
         struct tm *timeEvent = localtime(&rawtime);

         eventMsg = calloc(40,sizeof(char));
         sprintf(eventMsg, "%d,%s", eventCode, asctime(timeEvent));
         addElement(eventMsg,eventList);

         --numMsgs;
      }

      sendEvents(socketfd,eventList);

      endFile = fscanf(fd, "%d", &numMsgs) == EOF;
      sleep(10);
   }

   //ENVIAR LO QUE QUEDA

   fclose(fd);

   if (endFile) {
      printf("Format of file is incorrect.\n");
      exit(EXIT_FAILURE);
   }
}



void connectionServer(int *socketfd, struct sockaddr_in serv_addr) {

   int numTries;

   for (numTries = 99; numTries > -1; --numTries) {

      /* Intenta establecer una conexion al servidor */
      if((connect(*socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) && numTries > 0){
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

void createSocket(int *socketfd, char *address) {

   int n = 0;

   if ((*socketfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
      printf("Error: Could not create socket. \n");
      exit(EXIT_FAILURE);
   }

   struct sockaddr_in serv_addr;
   memset(&serv_addr, '0', sizeof(serv_addr));

   serv_addr.sin_family = AF_INET; //Asigando el tipo de direccion
   serv_addr.sin_port = htons(PUERTO); //Convierte el puerto a bits.

   /* Convierte el ip de str a formato de bits */
   if(inet_pton(AF_INET, address, &serv_addr.sin_addr) <= 0){
      printf("inet_pton error occured\n");
      exit(EXIT_FAILURE);
   }

   connectionServer(socketfd,serv_addr);
}

//svr_c -d <nombre_módulo_central> -p <puerto_svr_s> [-l <puerto_local>]
int main(int arg, char *argv[]) {

   char *path = malloc(sizeof(char)*100);
   path = "tests/msg1.txt";
   FILE *fd = fopen(path,"r");

   if (fd == NULL) {
      printf("File %s cannot be opened.", path);
      perror("");
      exit(EXIT_FAILURE);
   }

   int socketfd = 0;


   createSocket(&socketfd,argv[1]);

   struct list eventList;
   initializeList(&eventList);

   readEvents(fd,socketfd,&eventList);

   close(socketfd);

   return 0;
}
