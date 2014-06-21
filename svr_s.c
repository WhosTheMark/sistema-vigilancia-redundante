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
#include <pthread.h>
#include <signal.h>
#include "list.h"
#define MSGSIZE 81

pthread_mutex_t mutexList;
int execute;

struct atmThreadArgs {

   int socketfd;
   struct list *msgList;
};

struct logThreadArgs {

   FILE *logFile;
   struct list *msgList;
};

void usage() {

   printf("Usage: ./svr_s -l <puerto_svr_s> -b <archivo_bitacora>\n");
   exit(EXIT_FAILURE);
}

void terminateHandler(int signum) {

   execute = 0;
   printf("Exiting server...\n");

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
   *listenfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);

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

void *receiveMsgs(void *atmArgs) {

   struct atmThreadArgs *args = (struct atmThreadArgs *) atmArgs;
   int socketfd = args->socketfd;
   struct list *msgList = args->msgList;

   int n = 1;

   int msgSize = MSGSIZE * sizeof(char);

   while (n != 0) {

      char *msg = calloc(MSGSIZE,sizeof(char));
      memset(msg, '0', msgSize);

      n = read(socketfd, msg, msgSize-1);
      msg[n] = 0;

      pthread_mutex_lock(&mutexList);
      addElement(msg,msgList);
      pthread_mutex_unlock(&mutexList);

   }

   close(socketfd);
}

void *writeLog(void *logArgs) {

   struct logThreadArgs *args = (struct logThreadArgs *) logArgs;
   FILE *logFile = args->logFile;
   struct list *msgList = args->msgList;

   while (execute) {
      while (listSize(msgList) == 0);

      while (listSize(msgList) > 0) {

         pthread_mutex_lock(&mutexList);
         char *msg = getElement(msgList);
         pthread_mutex_unlock(&mutexList);

         printf("Event: %s\n",msg);
         fprintf(logFile,"%s\n",msg);
      }
   }

   fclose(logFile);
}

int main(int argc, char *argv[]) {

   char port[10];
   char *logPath = calloc(200,sizeof(char));

   serverArguments(argc,argv,port,logPath);

   FILE *log = NULL;
   int portNum;

   checkArguments(port,logPath,&portNum,&log);

   int listenfd = 0, *connfd;

   createSocket(&listenfd,portNum);

   // Escucha por un cliente.
   listen(listenfd, 128);

   struct list *msgList = calloc(1,sizeof(struct list));
   initializeList(msgList);

   pthread_t *atmThread, logThread;
   pthread_mutex_init(&mutexList,NULL);

   struct logThreadArgs *logArgs = calloc(1,sizeof(struct logThreadArgs));

   logArgs->logFile = log;
   logArgs->msgList = msgList;


   if (pthread_create(&logThread,NULL,writeLog,(void *) logArgs)) {
      printf("Error creating log thread.\n");
      exit(EXIT_FAILURE);
   }

   execute = 1;
   struct sigaction act;

   act.sa_handler = terminateHandler;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   sigaction(SIGINT,&act,NULL);

   while (execute) {

      connfd = calloc(1,sizeof(int));

      //Abre el socket con el cliente
      *connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

      struct atmThreadArgs *args = calloc(1,sizeof(struct atmThreadArgs));

      args->socketfd = *connfd;
      args->msgList = msgList;

      atmThread = calloc(1,sizeof(pthread_t));

      if (pthread_create(atmThread,NULL,receiveMsgs,(void *) args)) {

         printf("Error creating thread.\n");
         exit(EXIT_FAILURE);
      }
   }

   close(listenfd);


   return 0;
}