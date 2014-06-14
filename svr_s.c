#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#define PUERTO 6666

int main(int argc, char *argv[]) {

   int listenfd = 0, connfd = 0;
   struct sockaddr_in serv_addr;
   char buffer[1024];

   /* Se crea el welcoming socket */
   listenfd = socket(AF_INET, SOCK_STREAM, 0);

   // Se inicializa todo en 0 por si acaso.
   memset(&serv_addr, '0', sizeof(serv_addr));
   memset(buffer, '0', sizeof(buffer));

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //Coloca el ip de la maquina
   serv_addr.sin_port = htons(PUERTO); //Convierte el numero de puerto a bits

   // Asociar la informacion del registro con el socket
   bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

   // Escucha por un cliente.
   listen(listenfd, 128);

   //Abre el socket con el cliente
   connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

   int n = 1;

   while(n != 0) {

      n = read(connfd, buffer, sizeof(buffer)-1);
      buffer[n] = 0;
      printf("%s\n",buffer);
   }

   return 0;
}