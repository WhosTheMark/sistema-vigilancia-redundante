/**
 * @file codes.c
 * @author Marcos Campos 10-10108 
 * @author Andrea Salcedo 10-10666
 * @date 23 Jun 2014
 * @brief Archivo que contiene funciones para devolver el codigo de un evento.
 */

#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <string.h>

/** @brief Tamano del mensaje. */
#define MSGSIZE_CODE 200 
/** @brief Tamano de la fecha en el mensaje. */
#define DATESIZE 26 

/**
 * @brief Procedimiento que elimina los espacios en blanco de un string.
 * @param str String a modificar.
 */
void rmWhiteSpaces(char *str) {

   int i;

   for(i = strlen(str)-1; str[i] == ' '; --i);

   str[i+1] = '\0';
}

/**
 * @brief Funcion que verifica si el mensaje cumple con la expresion regular.
 * @param regexStr La expresion regular.
 * @param msg Mensaje a verificar.
 * @retval 0 El mensaje no cumple con la expresion regular.
 * @retval 1 El mensaje cumple con la expresion regular.
 */
  
int cmpRegex(char *regexStr, char *msg) {

   regex_t regex;

   if (regcomp(&regex,regexStr,0)) {
      printf("Error compiling regular expression.\n");
      return 0;
   }

   if (regexec(&regex,msg,0,NULL,0) == 0)
      return 1;

   regfree(&regex);
   return 0;
}

/**
 * @brief Devuelve el codigo del evento asociado a un mensaje.
 * @param originalMsg Mensaje de un evento.
 * @retval 0..21 Codigo del evento.
 * @retval -1 El mensaje no esta asociado a un evento valido.
 */
int getCode(char *originalMsg) {

   char **validMsgs = (char *[]) {  "the peripheral device is busy",
                                    "response of device not completely received",
                                    "device asked for termination",
                                    "application started",
                                    "ports verification",
                                    "communication offline",
                                    "device status change",
                                    "low cash alert",
                                    "service mode entered",
                                    "safe door opened",
                                    "top cassette removed",
                                    "top cassette inserted",
                                    "second cassette removed",
                                    "second cassette inserted",
                                    "third cassette removed",
                                    "third cassette inserted",
                                    "bottom cassette removed",
                                    "bottom cassette inserted",
                                    "safe door closed"
                                 };
   int i, NUMCODES = 19;

   char *event = originalMsg + DATESIZE; //Se elimina la fecha del mensaje original.
   char msg[MSGSIZE_CODE];

   /* Se copia el mensaje en minusculas. */
   for(i = 0; i < strlen(event); ++i)
      msg[i] = tolower(event[i]);

   msg[i] = '\0';

   rmWhiteSpaces(msg);

   /* Compara el mensaje copiado con los eventos en el arreglo de eventos.
    * Retorna la posicion (codigo del evento). */
   for(i = 0; i < NUMCODES; ++i) {

      if (strcmp(msg,validMsgs[i]) == 0)
         return i;
   }

   /* Si el mensaje no cumplio con ninguno de los eventos en el arreglo, entonces
    * se verifica si cumple con una de las siguientes expresiones regulares. */
   
   if (cmpRegex("^running out of notes in cassette [[:digit:]][[:digit:]]*$",msg))
      return NUMCODES;

   if (cmpRegex("^cassette [[:digit:]][[:digit:]]* empty$",msg))
      return ++NUMCODES;

   return -1; //Retorna -1 si el mensaje no esta asociado un evento valido.
}
