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
#include "codes.c"
#include "list.h"
#define MSGSIZE 80

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
   printf("\nExiting server...\n");

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

   if (*listenfd == -1) {
      printf("Error: creating listen socket.\n");
      exit(EXIT_FAILURE);
   }

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

   struct sockaddr_in addr;
   int addrLen;

   addr.sin_family = AF_INET;
   addrLen = sizeof(addr);
   addr.sin_addr.s_addr = 0L;
   getpeername(socketfd,(struct sockaddr *)&addr,&addrLen);

   char *ipAddr = inet_ntoa(addr.sin_addr);

   int n = 1;

   int msgSize = MSGSIZE * sizeof(char);
   char msg[MSGSIZE];

   while (n != 0 && execute) {

      memset(msg, 0, msgSize);

      n = read(socketfd, msg, msgSize-1);
      msg[n] = 0;

      int code = getCode(msg);

      if (code != -1) {

         printf("IP: %s, Code: %d, Event: %s\n",ipAddr,code,msg);
         char *eventMsg = calloc(200,sizeof(char));
         sprintf(eventMsg,"IP: %s, Event Code: %d, Description: %s.",ipAddr,code,msg);

         pthread_mutex_lock(&mutexList);
         addElement(eventMsg,msgList);
         pthread_mutex_unlock(&mutexList);
      }

   }

   free(args);

   close(socketfd);
}

void writeEvent(struct list *msgList, FILE *logFile) {

   while (listSize(msgList) > 0) {

      pthread_mutex_lock(&mutexList);
      char *msg = getElement(msgList);
      pthread_mutex_unlock(&mutexList);

      fprintf(logFile,"%s\n",msg);
   }
}

void *writeLog(void *logArgs) {

   struct logThreadArgs *args = (struct logThreadArgs *) logArgs;
   FILE *logFile = args->logFile;
   struct list *msgList = args->msgList;

   while (execute) {
      while (listSize(msgList) == 0 && execute)
         sleep(2);

      writeEvent(msgList,logFile);
   }

   if (listSize(msgList) > 0)
      writeEvent(msgList,logFile);

   free(args);
   fclose(logFile);
   pthread_exit(NULL);
}


void initializeServer(pthread_t *logThread, struct list *msgList, FILE *log) {

   execute = 1;
   struct sigaction act;
   act.sa_handler = terminateHandler;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   sigaction(SIGINT,&act,NULL);

   pthread_mutex_init(&mutexList,NULL);

   struct logThreadArgs *logArgs = calloc(1,sizeof(struct logThreadArgs));

   logArgs->logFile = log;
   logArgs->msgList = msgList;

   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);

   if (pthread_create(logThread,&attr,writeLog,(void *) logArgs)) {
      printf("Error creating log thread.\n");
      exit(EXIT_FAILURE);
   }

   pthread_attr_destroy(&attr);
}

void handleRequests(struct list *msgList, int listenfd) {

   while (execute) {

      int connfd = -1;

      //Abre el socket con el cliente
      connfd = accept(listenfd, (struct sockaddr*)NULL,NULL);

      if (!execute)
         break;

      if (connfd == -1 && errno != EINTR) {
         printf("Could not accept client.\n");
         continue;
      }

      struct atmThreadArgs *args = calloc(1,sizeof(struct atmThreadArgs));

      args->socketfd = connfd;
      args->msgList = msgList;

      pthread_t *atmThread = calloc(1,sizeof(pthread_t));

      if (pthread_create(atmThread,NULL,receiveMsgs,(void *) args)) {

         printf("Error creating thread.\n");
         exit(EXIT_FAILURE);
      }

   }
}


int main(int argc, char *argv[]) {

   char port[10];
   char logPath[200];

   serverArguments(argc,argv,port,logPath);

   FILE *log = NULL;
   int portNum;

   checkArguments(port,logPath,&portNum,&log);

   int listenfd = 0;

   createSocket(&listenfd,portNum);

   // Escucha por un cliente.
   listen(listenfd, 128);

   struct list *msgList = calloc(1,sizeof(struct list));
   initializeList(msgList);

   void *status;

   pthread_t logThread;

   initializeServer(&logThread,msgList,log);

   handleRequests(msgList,listenfd);


   if (pthread_join(logThread,&status))
      printf("Error joining thread.\n");

   close(listenfd);
   destroyList(msgList);
   free(msgList);

   return 0;
}

