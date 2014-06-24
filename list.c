/**
 * @file list.c
 * @author Marcos Campos 10-10108 
 * @author Andrea Salcedo 10-10666
 * @date 23 Jun 2014
 * @brief Archivo que contiene funciones para crear una lista dinamica.
 */

#ifndef LIST_
#define LIST_

#include <stdlib.h>
#include <stdio.h>

/** @brief Estructura de una lista. */
struct list {

   struct box *head; /**< La primer caja de la lista. */
   struct box *last; /**< La ultima caja de la lista. */
   struct box *secondLastBackup; /**< La penultima caja de respaldo. */ 
   struct box *lastBackup; /**< La ultima caja de respaldo. */
   int size; /**< Tamano de la lista. */
};

/** @brief Estructura de una caja de una lista. */
struct box {

   char *element; /**< String que contiene la caja. */
   struct box *next; /**< Apuntador a la siguiente caja en la lista. */
};

/**
 * @brief Inicializa una lista ya creada.
 * @param l Lista a inicializar.
 */
void initializeList(struct list *l) {

   l->head = NULL;
   l->last = NULL;
   l->secondLastBackup = NULL;
   l->lastBackup = NULL;
   l->size = 0;
}

/**
 * @brief Agrega el elemento dado a la lista dada.
 * @param element Elemento a agregar.
 * @param l Lista donde se agrega el elemento.
 */
void addElement(char *element, struct list *l) {

   struct box *elemBox = calloc(1,sizeof(struct box));

   elemBox->element = element;
   elemBox->next = NULL;

   ++(l->size);

   if (l->head == NULL) {

      l->head = elemBox;
      l->last = elemBox;

   } else {

      l->last->next = elemBox;
      l->last = elemBox;
   }
}

/**
 * @brief Devuelve el elemento de la primera caja de la lista.
 * @param l Lista de donde se devuelve el elemento.
 * @retval NULL Si la lista esta vacia.
 * @retval char* El elemento de la primera caja.
 */ 
char *getElement(struct list *l) {

   if (l->head == NULL)
      return NULL;

   struct box *elemBox = l->head;
   l->head = l->head->next;
   --(l->size);

   if (l->size == 0)
      l->last = NULL;

   char *elem = elemBox->element;

   /* Si el penultimo respaldo no es vacio y no es igual a la caja que contiene
    * el elemento a retornar entonces se libera la memoria. */
   if (l->secondLastBackup != NULL && l->secondLastBackup != elemBox) {
      free(l->secondLastBackup->element);
      free(l->secondLastBackup);
   }

   l->secondLastBackup = l->lastBackup; //El ultimo respaldo ahora es el penultimo.
   l->lastBackup = elemBox; //Se respalda la caja del elemento a retornar.

   return elem;
}

/**
 * @brief Devuelve el tamano de la lista.
 * @param l La lista.
 * @return Tamano de la lista.
 */
int listSize(struct list *l) {

   return l->size;
}

/**
 * @brief Agrega el penultimo respaldo nuevamente a la lista.
 * @param l La lista.
 */
void restoreBackup(struct list *l) {

   if (l->secondLastBackup != NULL) {

      l->secondLastBackup->next = l->head;
      l->head = l->secondLastBackup;
      ++(l->size);

      if (l->size == 1)
         l->last = l->head;
   }
}

/**
 * @brief Destructor de la lista.
 * @param l La lista a liberar.
 */
void destroyList(struct list *l){

   while (l->head != NULL) {
      struct box *boxHelper = l->head;
      l->head = boxHelper->next;
      free(boxHelper->element);
      free(boxHelper);
   }

   if (l->secondLastBackup != NULL) {
      free(l->secondLastBackup->element);
      free(l->secondLastBackup);
   }

   if (l->lastBackup != NULL) {
      free(l->lastBackup->element);
      free(l->lastBackup);
   }
}

#endif