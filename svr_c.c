/**
 * @file svr_c.c
 * @author Marcos Campos 10-10108 
 * @author Andrea Salcedo 10-10666
 * @date 23 Jun 2014
 * @brief Archivo con las funcionalidades del ATM en el SVR.
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
#include <sys/types.h>
#include <limits.h>
#include <netdb.h>
#include <pthread.h>
#include <curl/curl.h>
#include <ifaddrs.h>
#include "list.h"
#include "mail.c"

/** @brief Numero de intentos que el ATM realiza para volverse conectar al servidor. */
#define NUMTRIES 9
/** @brief Tamano de un evento. */
#define EVENTSIZE 50
/** @brief Tamano de un mensaje. */
#define MSGSIZE 80

/** @brief Semaforo para manejar concurrencia con la lista de eventos. */
pthread_mutex_t mutexList;

/** @brief Global para determinar si se debe cerrar la aplicacion*/
int execute;

/** @brief Argumentos del hilo que envia los mensajes al servidor. */
struct sendThreadArgs {

   char *remotePort; /**< El numero de puerto remoto. */
   int localPort; /**< El numero de puerto local. */
   char *domain; /**< El nombre del dominio del servidor. */
   struct list *eventList; /**< La lista que contiene los eventos del ATM. */
   CURL *curl; /**< Estructura para mandar correos. */
   
};

/** @brief Imprime por pantalla como iniciar la aplicacion. */
void usage() {

   printf("Usage: ./svr_c -d <nombre_modulo_central> -p <puerto_svr_s> ");
   printf("[-l <puerto_local>]\n");
   exit(EXIT_FAILURE);
}

/**
 * @brief Verifica si los argumentos del ATM fueron ingresados correctamente.
 * @param argc Numero de argumentos.
 * @param argv Arreglo con los argumentos ingresados.
 * @param domain String donde se almacena el dominio/IP del servidor.
 * @param remotePort String donde se almacena el puerto remoto del servidor.
 * @param localPort String donde se almacena el puerto local.
 */
void clientArguments(int argc, char *argv[], char *domain, 
                     char *remotePort, char *localPort) {

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

   /* Verifica que al menos se ingresaron los argumentos obligatorios y
    * que cada tipo de argumento fue ingresado solamente una vez. */
   if (flagD != 1 || flagP != 1 || flagL > 1) {
      printf("Incorrect arguments for initiating client.\n");
      usage();
   }
}

/**
 * @brief Crea un mensaje con la hora actual dado un evento.
 * @param eventMsg String del evento.
 * @return Mensaje con la hora y el evento.
 */
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
   
   /* Elimina el salto de linea al final del mensaje y se agrega \0. */
   if (msg[lengthEvent-1] == '\n')
      msg[lengthEvent-1] = ' ';
   msg[MSGSIZE-1] = '\0';
   
   printf("Message to send: %s\n", msg);

   return msg;
}

char *getIP() {
   
   struct ifaddrs *addrs;
   
   if (getifaddrs(&addrs) == -1) {
      printf("Error getting IP address.\n");
      return "";
   }
   
   while (addrs) {
      
      if (addrs->ifa_addr && addrs->ifa_addr->sa_family == AF_INET) {       
         struct sockaddr_in *addr = (struct sockaddr_in *) addrs->ifa_addr;
         return inet_ntoa(addr->sin_addr);
      }      
      addrs = addrs->ifa_next;
   }   
   return "";   
}

/**
 * @brief Procedimiento para conectar el ATM con el servidor.
 * @param socketfd El file descriptor del socket ya creado.
 * @param addr Estructura que contiene informacion del socket.
 * @param eventList Lista que almacena los eventos del ATM.
 */
void connectionServer(int *socketfd, struct sockaddr addr, struct list *eventList,
                      CURL *curl) {

   int numTries;

   for (numTries = NUMTRIES; numTries > -1; --numTries) {

      /* Intenta establecer una conexion al servidor. */
      if (connect(*socketfd,&addr, sizeof(addr)) < 0){
         
         printf("Connection to server failed. Trying again...\n");
         sleep(1);
      } else
         break;

      /* Cada 10 intentos, se agrega el mensaje actual a la lista de eventos
       * y el ATM espera 30 segundos para intentar conectarse con el
       * servidor nuevamente. */
      if (numTries % 10 == 0) {

         char *msg = createMsg("communication offline");

         pthread_mutex_lock(&mutexList);
         addElement(msg,eventList);
         pthread_mutex_unlock(&mutexList);

         printf("Connection to server failed. Trying again in 30 seconds.\n");
         sleep(30);
      }
   }

   /* Si el ATM no se puede conectar despues del maximo numero de intentos,
    * se envia un correo de la falla y se termina el programa. */
   if (numTries == -1) {
      
      printf("Connection to server failed. ");
      printf("Sending email to supervisor. This may take a few minutes.\n");

      char *ipAddr = getIP(), message[MSGSIZE];
            
      sprintf(message,"IP: %s, Message: %s.",ipAddr,"Connection to server failed"); 
      
      setMailContent(curl,message);
      sendMail(curl);
      
      exit(EXIT_FAILURE);
   }

   printf("Connection established.\n");
}

/**
 * @brief Crea un socket.
 * @param remotePort String que contiene el puerto remoto del servidor.
 * @param localPort Numero de puerto local.
 * @param socketfd Donde se almacena el file descriptor del socket a crear.
 * @param domain El dominio/IP del servidor.
 * @param addr Estructura donde se almacena informacion pertinente al socket.
 */
void createSocket(char *remotePort, int localPort, int *socketfd, 
                  char *domain, struct sockaddr *addr) {

   struct addrinfo hints, *servInfo, *p;

   memset(&hints,0,sizeof(hints));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   /* Si el dominio/IP dado no es valido, se termina el programa. */
   if (getaddrinfo(domain,remotePort,&hints,&servInfo) != 0) {
      printf("%s is not a valid domain.\n",domain);
      exit(EXIT_FAILURE);
   }
   
   /* Si no se puede crear el socket, se termina el programa. */
   if ((*socketfd = socket(servInfo->ai_family, servInfo->ai_socktype,
      servInfo->ai_protocol)) == -1) {

      printf("Error: could not create socket.\n");
      exit(EXIT_FAILURE);
   }

   *addr = *(servInfo->ai_addr);

   /* Si se ingreso un puerto local al iniciar el modulo cliente, 
    * entonces se asocia al socket creado. */
   if (localPort != -1) {

      struct sockaddr_in localAddr;

      localAddr.sin_family = AF_INET;
      localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
      localAddr.sin_port = htons(localPort);

      bind(*socketfd, (struct sockaddr *)&localAddr, sizeof(localAddr));

      /* Si el puerto local dado ya se esta utilizando, 
       * se termina el programa. */
      if (errno == EADDRINUSE) {
         printf("Error: port #%d is already in use.\n",localPort);
         exit(EXIT_FAILURE);
      }
   }
   free(servInfo);
}

/**
 * @brief Procedimiento para que el ATM pueda reconectarse al servidor.
 * @param socketfd El file descriptor del socket.
 * @param remotePort El numero de puerto remoto del servidor.
 * @param localPort  El numero de puerto local.
 * @param domain El dominio/IP del servidor.
 * @param event El ultimo evento realizado en el ATM antes de caerse la conexion.
 * @param eventList La lista con los eventos realizados en el ATM.
 */  
void reconnect(int *socketfd, char *remotePort, int localPort, char *domain, 
               char *event, struct list *eventList, CURL *curl) {

   close(*socketfd); //Cierra el file descriptor actual.
   
   struct sockaddr addr;
   printf("Server was down. Reconnecting...\n");

   /* Crea nuevamente un socket e intenta conectarse al servidor. */
   createSocket(remotePort,localPort,socketfd,domain,&addr);
   connectionServer(socketfd,addr,eventList,curl);

   /* Se recupara el penultimo evento respaldado en la lista 
    * y se agrega el ultimo evento a lista. */
   pthread_mutex_lock(&mutexList);
   restoreBackup(eventList);
   addElement(event,eventList);
   pthread_mutex_unlock(&mutexList);

}

/**
 * @brief Auxiliar para mandar eventos realizados en el ATM.
 * @param remotePort El numero de puerto remoto del servidor.
 * @param localPort El numero de puerto local.
 * @param domain El dominio/IP del servidor.
 * @param socketfd El file descriptor del socket conectado con el servidor.
 * @param eventList La lista con los eventos realizados en el ATM.
 */
void sendEventsHelper(char *remotePort, int localPort, char *domain, 
                      int *socketfd, struct list *eventList, CURL *curl) {

   char *event;

   while (listSize(eventList) > 0) {

      pthread_mutex_lock(&mutexList);
      event = getElement(eventList);
      pthread_mutex_unlock(&mutexList);

      int numTries = 0;
      errno = 0;
      
      int numBytes = send(*socketfd,event,strlen(event),MSG_NOSIGNAL);

      /* Si se intenta de enviar el evento y pipe esta roto entonces se 
       * intenta de restablecer la conexion. */
      if (errno == EPIPE) { 
         reconnect(socketfd,remotePort,localPort,domain,event,eventList,curl);

      /* Si existe otro error al enviar el evento entonces se sigue 
       * intentando de enviar un maximo de 10 veces. */
      } else if (numBytes == -1) {

         while (numBytes < 0 && numTries < 10) {

            printf("Error sending message. Trying again...\n");

            errno = 0;
            numBytes = send(*socketfd,event,strlen(event),MSG_NOSIGNAL);

            ++numTries;
            sleep(1);
         }

         /* Si el ATM no pudo enviar el mensaje despues de 10 intentos 
          * entonces intenta de restablecer la conexion con el servidor.*/
         if (numTries == 10) 
            reconnect(socketfd,remotePort,localPort,domain,event,eventList,curl);
         
      }
   }
}

/**
 * @brief Procedimiento del hilo que envia los eventos realizados por el ATM.
 * @param sendArgs Argumentos asociados al hilo.
 * <br> remotePort - Contiene el numero de puerto remoto del servidor.
 * <br> localPort - El numero de puerto local.
 * <br> domain - Dominio/IP del servidor.
 * <br> evenList - La lista de los eventos realizados por el cliente.
 */ 
void *sendEvents(void *sendArgs) {

   struct sendThreadArgs *args = (struct sendThreadArgs *) sendArgs;

   char *remotePort = args->remotePort;
   int localPort = args->localPort;
   char *domain = args->domain;
   struct list *eventList = args->eventList;
   CURL *curl = args->curl;

   int socketfd;
   struct sockaddr addr;

   /* Crea el socket y se conecta al servidor. */
   createSocket(remotePort,localPort,&socketfd,domain,&addr);
   connectionServer(&socketfd,addr,eventList,curl);

   while (execute) {

      while (listSize(eventList) == 0 && execute)
         sleep(2);

      /* Se envian los elementos que estan almacenados en la lista. */
      sendEventsHelper(remotePort,localPort,domain,&socketfd,eventList,curl);
   }

   /* Si al salirse del programa, todavia hay eventos almacenados en la lista
    * entonces se mandan y luego se cierra el hilo. */
   if (listSize(eventList) > 0)
      sendEventsHelper(remotePort,localPort,domain,&socketfd,eventList,curl);

   pthread_exit(NULL);
}

/**
 * @brief Procedimiento del hilo que lee los eventos realizados por el ATM.
 * @param eList Lista donde se almacenan los eventos realizados por el ATM.
 */
void *readEvents(void *eList) {

   struct list *eventList = (struct list *) eList;
   
   int lenEvent = EVENTSIZE;
   char *eventMsg = calloc(EVENTSIZE,sizeof(char));

   while (1) {

      /* Lee de la entrada estandar el evento. */
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

/**
 * @brief Convierte un string con un numero de puerto a un entero.
 * @param portStr String con el numero de puerto.
 * @return El numero de puerto.
 */
int portStrToNum(char *portStr) {

   errno = 0;
   char *endptr;

   /* Verifica que el string no sea vacio. */
   if (strcmp(portStr,"") == 0)
      return -1;

   int port = strtol(portStr,&endptr,10);

   /* Verifica si es un puerto valido. */
   if ((errno == ERANGE || (errno != 0 && port == 0)) || (endptr == portStr) ||
      (*endptr != '\0') || (port < 1) || (port > USHRT_MAX)) {

      printf("Invalid port number.\n");
      exit(EXIT_FAILURE);
   }

   return port;
}

/**
 * @brief Inicializa el modulo cliente.
 * @param readThread Hilo encargado de leer los eventos del ATM del stdio.
 * @param sendThread Hilo encargado de enviar los eventos al servidor.
 * @param sendArgs Estructura con los argumentos del hilo que envia eventos.
 * @param eventList Lista donde se almacena los eventos realizados por el ATM.
 */
void intializeClient(pthread_t *readThread, pthread_t *sendThread,
                     struct sendThreadArgs sendArgs, struct list *eventList) {

   execute = 1;

   /* Inicializacion de los atributos de sendThread 
    * para que este hilo sea joinable. */
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);

   /* Se inicializa el semaforo de la lista de eventos. */
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

/**
 * @brief Inicializa la aplicacion y ejecuta las funcionalidades del ATM.
 * @param argc Numero de argumentos ingresados al iniciar la aplicacion.
 * @param argv Arreglo de los argumentos ingresados.
 */
int main(int argc, char *argv[]) {

   char *domain = calloc(100,sizeof(char));
   char *remotePort = calloc(10,sizeof(char));
   char *localPort = calloc(10,sizeof(char));

   CURL *curl = curl_easy_init();
   
   if (!curl) {   
      printf("Error creating curl.\n");
      exit(EXIT_FAILURE);
   }
   
   setMailSettings(curl);
   
   /* Verifica los argumentos. */
   clientArguments(argc,argv,domain,remotePort,localPort);

   int localPortNum = portStrToNum(localPort);

   struct list eventList;
   initializeList(&eventList);

   /* Se agrega el mensaje de que se inicio la aplicacion
    * a la lista de eventos. */
   char *msg = createMsg("application started");
   addElement(msg,&eventList);

   void *status;

   struct sendThreadArgs sendArgs;

   /* Inicializacion de la estructura que utiliza el hilo de enviar eventos. */
   sendArgs.remotePort = remotePort;
   sendArgs.localPort = localPortNum;
   sendArgs.domain = domain;
   sendArgs.eventList = &eventList;
   sendArgs.curl = curl;

   pthread_t *readThread = calloc(1,sizeof(pthread_t));
   pthread_t *sendThread = calloc(1,sizeof(pthread_t));

   intializeClient(readThread,sendThread,sendArgs,&eventList);

   while (execute)
      sleep(2);

   /* Espera que termine el hilo que envia eventos. */
   if (pthread_join(*sendThread,&status))
      printf("Error joining thread.\n");

   /* Liberacion de memoria. */
   free(readThread);
   free(sendThread);
   free(domain);
   free(remotePort);
   free(localPort);
   destroyList(&eventList);
   curl_easy_cleanup(curl);

   return 0;
}
