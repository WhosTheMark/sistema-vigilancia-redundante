#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "list.h"
#define PUERTO 6666

void readEvents(FILE *fd, int socketfd) {

   int numMsgs;

   int endFile = fscanf(fd, "%d", &numMsgs) == EOF;

   while (!endFile && numMsgs != 0) {

      int eventCode;
      char eventMsg[40];

      printf("Sending messages\n");
      sprintf(eventMsg,"%d",numMsgs);
      write(socketfd,eventMsg,strlen(eventMsg));

      while (!endFile && numMsgs != 0) {

         endFile = fscanf(fd, "%d", &eventCode) == EOF;

         time_t rawtime = time(NULL);
         struct tm *timeEvent = localtime(&rawtime);

         sprintf(eventMsg, "%d,%s", eventCode, asctime(timeEvent));
         write(socketfd,eventMsg,strlen(eventMsg));

         --numMsgs;
      }

      endFile = fscanf(fd, "%d", &numMsgs) == EOF;
      sleep(10);
   }

   fclose(fd);

   if (endFile) {
      printf("Format of file is incorrect.\n");
      exit(-1);
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
      exit(-1);
   }
}

void createSocket(int *socketfd, char *address) {

   int n = 0;

   if ((*socketfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
      printf("Error: Could not create socket. \n");
      exit(-1);
   }

   struct sockaddr_in serv_addr;
   memset(&serv_addr, '0', sizeof(serv_addr));

   serv_addr.sin_family = AF_INET; //Asigando el tipo de direccion
   serv_addr.sin_port = htons(PUERTO); //Convierte el puerto a bits.

   /* Convierte el ip de str a formato de bits */
   if(inet_pton(AF_INET, address, &serv_addr.sin_addr) <= 0){
      printf("inet_pton error occured\n");
      exit(-1);
   }

   connectionServer(socketfd,serv_addr);
}


int main(int arg, char *argv[]) {

   char *path = malloc(sizeof(char)*100);
   path = "pruebas/msg1.txt";
   FILE *fd = fopen("pruebas/msg1.txt","r");

   if (fd == NULL) {
      printf("File %s cannot be opened.", path);
      perror("");
      exit(-1);
   }

   int socketfd = 0;

   createSocket(&socketfd,argv[1]);

   readEvents(fd,socketfd);

   close(socketfd);

   return 0;
}
