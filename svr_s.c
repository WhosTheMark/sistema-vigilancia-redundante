/**
 * @file svr_s.c
 * @author Marcos Campos 10-10108 
 * @author Andrea Salcedo 10-10666
 * @date 23 Jun 2014
 * @brief Archivo que contiene las funcionalidades del servidor del SVR.
 */

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
#include <curl/curl.h>
#include "mail.c"
#include "codes.c"
#include "list.h"

/** @brief Tamano maximo de un mensaje */
#define MSGSIZE 80

/** @brief Semaforo que maneja la lista de los mensajes recibidos. */
pthread_mutex_t mutexList;

/** @brief Semaforo que maneja la lista de los correos que debe mandar el servidor. */
pthread_mutex_t mutexEmail;

/** @brief Global para determinar si se debe cerra el servidor. */
int execute;

/** @brief Argumentos de los hilos que atienden a los ATMs. */
struct atmThreadArgs {

   int socketfd; /**< El file descriptor del socket asociado al ATM atendido.*/
   struct list *msgList; /**< Lista de los mensajes recibidos. */
   struct list *emailList; /**< Lista de los correos a enviar. */   
};

/** @brief Argumentos del hilo que escribe en la bitacora. */
struct logThreadArgs {

   FILE *logFile; /**< Archivo de la bitacora. */
   struct list *msgList; /**< Lista de los mensajes recibos por el servidor. */
};

/** @brief Argumentos del hilo que envia correos. */
struct emailThreadArgs {
  
   struct list *emailList; /**< Lista de los correos a enviar. */
   CURL *curl; /**< Estructura para mandar correos. */
};

/** @brief Imprime por pantalla como iniciar el servidor. */
void usage() {

   printf("Usage: ./svr_s -l <puerto_svr_s> -b <archivo_bitacora>\n");
   exit(EXIT_FAILURE);
}

/**
 * @brief Manejador de interrupciones para cerrar el servidor.
 * Cambia el valor de la global execute a 0 para que el servidor pueda 
 * cerrar los procesos y liberar memoria antes de terminar. 
 * @param signum El numero de la interrupcion.
 */
void terminateHandler(int signum) {

   execute = 0;
   printf("\nExiting server...\n");
}

/**
 * @brief Verifica si los argumentos del servidor fueron ingresados correctamente.
 * @param argc Numero de argumentos.
 * @param argv Arreglo con los argumentos ingresados.
 * @param port String donde se almacena el puerto del welcoming socket.
 * @param log Ruta del archivo de la bitacora.
 */
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

   /* Verifica que todos los argumentos fueron ingresados solamente una vez. */
   if (flagL != 1 || flagB != 1) {
      printf("Incorrect arguments for initiating server.\n");
      usage();
   }
}

/**
 * @brief Verifica los argumentos ingresados al iniciar el servidor.
 * @param port String que contiene el numero del puerto del welcoming socket.
 * @param logPath Ruta del archivo de la bitacora.
 * @param portNum Entero con el numero de puerto.
 * @param log Archivo de la bitacora.
 */
void checkArguments(char *port, char *logPath, int *portNum, FILE **log) {

   errno = 0;
   char *endptr;
   *portNum = strtol(port,&endptr,10); //Convierte a entero el numero de puerto.

   /* Verifica que el puerto dado es valido. */
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

/**
 * @brief Crea el welcoming socke del servidor.
 * @param listenfd El file descriptor del welcoming socket.
 * @param portNum El numero de puerto del welcoming socket.
 */
void createSocket(int *listenfd, int portNum) {

   struct sockaddr_in serv_addr;

   /* Se crea el welcoming socket. */
   *listenfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);

   if (*listenfd == -1) {
      printf("Error creating listen socket.\n");
      exit(EXIT_FAILURE);
   }

   memset(&serv_addr, '0', sizeof(serv_addr)); //Inicializacion del struct.

   serv_addr.sin_family = AF_INET; //Se utiliza IPv4
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //Coloca el IP de la maquina
   serv_addr.sin_port = htons(portNum); //Convierte el numero de puerto a bits

   /* Asocia la informacion del struct con el welcoming socket. */
   bind(*listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

   /* Verifica si el puerto ya se esta utilizando. */
   if (errno == EADDRINUSE) {
      printf("Error: port #%d is already in use.\n",portNum);
      exit(EXIT_FAILURE);
   }
}

/**
 * @brief Procedimiento de los hilos que atienden los ATMs.
 * @param atmArgs Argumentos del hilo.
 * <br> socketfd - File descriptor del socket asociado al hilo.
 * <br> msgList - Lista de los mensajes que recibe el servidor.
 */
void *receiveMsgs(void *atmArgs) {

   struct atmThreadArgs *args = (struct atmThreadArgs *) atmArgs;
   int socketfd = args->socketfd;
   struct list *msgList = args->msgList;
   struct list *emailList = args->emailList;

   struct sockaddr_in addr;
   int addrLen;

   addr.sin_family = AF_INET;
   addrLen = sizeof(addr);
   addr.sin_addr.s_addr = 0L;
   
   /* Obtiene el IP del ATM al cual esta atendiendo.*/
   getpeername(socketfd,(struct sockaddr *)&addr,&addrLen);

   char *ipAddr = inet_ntoa(addr.sin_addr);

   int n = 1;

   int msgSize = MSGSIZE * sizeof(char);
   char msg[MSGSIZE];

   while (n != 0 && execute) {

      memset(msg, 0, msgSize);

      /* Lee los mensajes del ATM. */
      n = read(socketfd, msg, msgSize-1);
      msg[n] = 0;

      int code = getCode(msg);

      if (code != -1) {

         char *eventMsg = calloc(200,sizeof(char));
         
         sprintf(eventMsg,"IP: %s, Event Code: %d, Description: %s.",ipAddr,code,msg);
         printf("%s\n",eventMsg);

         /* Almacena el mensaje del evento en la lista de mensajes recibidos.*/
         pthread_mutex_lock(&mutexList);
         addElement(eventMsg,msgList);
         pthread_mutex_unlock(&mutexList);
         
         if (needAlert(code)) {
            
            pthread_mutex_lock(&mutexEmail);
            addElement(eventMsg,emailList);
            pthread_mutex_unlock(&mutexEmail);
         }
      }
   }
   free(args);
   close(socketfd);
}

/**
 * @brief Escribe los mensajes recibidos en la bitacora.
 * @param msgList Lista de los mensajes recibidos por el servidor.
 * @param logFile Archivo de la bitacora.
 */
void writeEvent(struct list *msgList, FILE *logFile) {

   while (listSize(msgList) > 0) {

      pthread_mutex_lock(&mutexList);
      char *msg = getElement(msgList);
      pthread_mutex_unlock(&mutexList);

      fprintf(logFile,"%s\n",msg);
   }
}

/**
 * @brief Procedimiento del hilo que escribe en la bitacora del servidor.
 * @param logArgs Argumentos del hilo.
 * <br> logFile - Archivo de la bitacora.
 * <br> msgList - Lista con los mensajes recibidos por el servidor.
 */
void *writeLog(void *logArgs) {

   struct logThreadArgs *args = (struct logThreadArgs *) logArgs;
   FILE *logFile = args->logFile;
   struct list *msgList = args->msgList;

   while (execute) {
      while (listSize(msgList) == 0 && execute)
         sleep(2);

      writeEvent(msgList,logFile);
   }

   /* Antes de cerrar el servidor, el hilo envia los mensajes que 
    * todavia estan en la lista. */
   if (listSize(msgList) > 0)
      writeEvent(msgList,logFile);

   free(args);
   fclose(logFile);
   pthread_exit(NULL);
}


void sendEmail(struct list *emailList, CURL *curl) {
 
   while (listSize(emailList) > 0) {
      pthread_mutex_lock(&mutexEmail);
      char *msg = getElement(emailList);
      pthread_mutex_unlock(&mutexEmail);
      
      printf("Suspicious pattern found. Sending an email to the supervisor.\n");
      setMailContent(curl,msg);
      sendMail(curl);
   }
}


void *sendEmails(void *emailArgs) {
 
   struct emailThreadArgs *args = (struct emailThreadArgs *) emailArgs;
   struct list *emailList = args->emailList;
   CURL *curl = args->curl;
   
   while (execute) {
      
      while (listSize(emailList) == 0 && execute)
         sleep(2);
      
      sendEmail(emailList,curl);
   }
   
   sendEmail(emailList,curl);
   
   free(args);
   curl_easy_cleanup(curl);
   pthread_exit(NULL);
}


/**
 * @brief Inicializa el modulo servidor.
 * @param logThread Hilo que maneja la bitacora.
 * @param msgList Lista de los mensajes que recibe el servidor.
 * @param log Archivo de la bitacora.
 */
void initializeServer(pthread_t *logThread, struct list *msgList, FILE *log,
                      pthread_t *emailThread, struct list *emailList, CURL *curl) {

   /* Se reemplaza el manejador de interrupciones para que el servidor pueda
    * cerrarse adecuadamente al presionar ctrl+c. */
   execute = 1;
   struct sigaction act;
   act.sa_handler = terminateHandler;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   sigaction(SIGINT,&act,NULL);

   /* Inicializacion del semaforo para la lista de mensajes recibidos. */
   pthread_mutex_init(&mutexList,NULL);

    /* Inicializacion del semaforo para la lista de correos a mandar. */
   pthread_mutex_init(&mutexEmail,NULL);

   struct logThreadArgs *logArgs = calloc(1,sizeof(struct logThreadArgs));

   logArgs->logFile = log;
   logArgs->msgList = msgList;
   
   struct emailThreadArgs *emailArgs = calloc(1,sizeof(struct emailThreadArgs));
   
   emailArgs->emailList = emailList;
   emailArgs->curl = curl;
   
   /* Inicializacion de los atributos del hilo que maneja la 
    * bitacora para que este hilo sea joinable. */
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);

   if (pthread_create(logThread,&attr,writeLog,(void *) logArgs)) {
      printf("Error creating log thread.\n");
      exit(EXIT_FAILURE);
   }
   
   if (pthread_create(emailThread,&attr,sendEmails,(void *) emailArgs)) {
      printf("Error creating email thread.\n");
      exit(EXIT_FAILURE);
   }

   pthread_attr_destroy(&attr);
}

/**
 * @brief Maneja las solicitudes de los ATMs que desean conectarse al servidor.
 * @param msgList Lista de los mensajes recibidos por el servidor.
 * @param listenfd El file descriptor del welcoming socket.
 */
void handleRequests(struct list *msgList, struct list *emailList, int listenfd) {

   while (execute) {

      int connfd = -1;

      /* Acepta y abre el socket con el cliente. */
      connfd = accept(listenfd, (struct sockaddr*)NULL,NULL);

      if (!execute)
         break;

      if (connfd == -1 && errno != EINTR) {
         printf("Error: could not accept client.\n");
         continue;
      }

      struct atmThreadArgs *args = calloc(1,sizeof(struct atmThreadArgs));

      args->socketfd = connfd;
      args->msgList = msgList;
      args->emailList = emailList;

      pthread_t *atmThread = calloc(1,sizeof(pthread_t));

      /* Crea un hilo para atender y escuchar los mensajes del cliente. */
      if (pthread_create(atmThread,NULL,receiveMsgs,(void *) args)) {

         printf("Error creating thread.\n");
         exit(EXIT_FAILURE);
      }
   }
}

/**
 * @brief Inicializa la aplicacion y ejecuta las funcionalidades del servidor.
 * @param argc Numero de argumentos ingresados al iniciar el servidor.
 * @param argv Arreglo de los argumentos ingresados.
 */
int main(int argc, char *argv[]) {

   char port[10];
   char logPath[200];
   CURL *curl = curl_easy_init();
   
   if (!curl) {   
      printf("Error creating curl.\n");
      exit(EXIT_FAILURE);
   }
   
   setMailSettings(curl);

   serverArguments(argc,argv,port,logPath);

   FILE *log = NULL;
   int portNum;

   checkArguments(port,logPath,&portNum,&log);

   int listenfd = 0;

   createSocket(&listenfd,portNum);

   listen(listenfd, 128); //Escucha por un cliente.

   /* Crea una lista para los mensajes que va recibir el servidor
    * y la inicializa. */
   struct list *msgList = calloc(1,sizeof(struct list));
   initializeList(msgList);
   
   /* Crea una lista para los correos que debe mandar el servidor. */
   struct list *emailList = calloc(1,sizeof(struct list));
   initializeList(emailList);

   void *status;

   pthread_t logThread, emailThread;

   /* Inicializa el servidor. */
   initializeServer(&logThread,msgList,log,&emailThread,emailList,curl); 

   handleRequests(msgList,emailList,listenfd); // Maneja las solicitudes de los ATMs.

   /* Espera que termine el hilo encargado de la bitacora antes de cerrar. */
   if (pthread_join(logThread,&status))
      printf("Error joining thread.\n");

   /* Espera que termine el hilo encargado de enviar correos antes de cerrar. */
   if (pthread_join(emailThread,&status))
      printf("Error joining thread.\n");
   
   close(listenfd);
   destroyList(msgList);
   free(msgList);

   return 0;
}

