#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

void usage() {

   printf("Usage: ./svr_s -l <puerto_svr_s> -b <archivo_bitacora>\n");
   exit(EXIT_FAILURE);
}

void serverArguments(int argc, char *argv[], char *port, char *log) {

   int flagL = 0, flagB = 0;

   while((argc > 1) && (argv[1][0] == '-')) {

      switch (argv[1][1]) {

         case 'l':

            if (argc > 2)
               strcpy(port,argv[2]);
            else {
               printf("The flag -l requires an argument.\n");
               usage();
            }
            ++flagL;
            break;

         case 'b':

            if (argc > 2)
               strcpy(log,argv[2]);
            else {
               printf("The flag -b requires an argument.\n");
               usage();
            }
            ++flagB;
            break;

         default:

            printf("Incorrect arguments for initiating server.\n");
            usage();
      }
      argv += 2;
      argc -= 2;
   }

   if (flagL != 1 || flagB != 1) {
      printf("Incorrect arguments for initiating server.\n");
      usage();
   }
}

void checkArguments(char *port, char *logPath, int *portNum, FILE **log) {

   errno = 0;
   char *endptr;
   *portNum = strtol(port,&endptr,10);

   if ((errno == ERANGE || (errno != 0 && *portNum == 0)) || (endptr == port) ||
      (*endptr != '\0') || (*portNum < 1) || (*portNum > USHRT_MAX)) {

      printf("Invalid port number.\n");
      exit(EXIT_FAILURE);
   }

   *log = fopen(logPath,"a+");

   if (*log == NULL) {
      printf("File %s cannot be opened.", logPath);
      perror("");
      exit(EXIT_FAILURE);
   }
}

void createSocket(int *listenfd, int portNum) {

   struct sockaddr_in serv_addr;

   /* Se crea el welcoming socket */
   *listenfd = socket(AF_INET, SOCK_STREAM, 0);

   // Se inicializa todo en 0 por si acaso.
   memset(&serv_addr, '0', sizeof(serv_addr));

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //Coloca el ip de la maquina
   serv_addr.sin_port = htons(portNum); //Convierte el numero de puerto a bits

   // Asociar la informacion del registro con el socket
   bind(*listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

   if (errno == EADDRINUSE) {
      printf("Error: port #%d is already in use.\n",portNum);
      exit(EXIT_FAILURE);
   }
}

int main(int argc, char *argv[]) {

   char port[10];
   char *logPath = calloc(200,sizeof(char));

   serverArguments(argc,argv,port,logPath);

   FILE *log = NULL;
   int portNum;

   checkArguments(port,logPath,&portNum,&log);

   int listenfd = 0, connfd = 0;
   char buffer[1024];

   memset(buffer, '0', sizeof(buffer));

   createSocket(&listenfd,portNum);

   // Escucha por un cliente.
   listen(listenfd, 128);

   //Abre el socket con el cliente
   connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

   int n = 1;

   while (n != 0) {

      n = read(connfd, buffer, sizeof(buffer)-1);
      buffer[n] = 0;
      printf("%s\n",buffer);
   }

   close(connfd);
   close(listenfd);

   if (log != NULL)
      fclose(log);

   return 0;
}