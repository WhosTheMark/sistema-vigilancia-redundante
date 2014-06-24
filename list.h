/**
 * @file list.h
 * @author Marcos Campos 10-10108 
 * @author Andrea Salcedo 10-10666
 * @date 23 Jun 2014
 * @brief Archivo que contiene funciones para crear una lista dinamica.
 */

#ifndef LIST_H_
#define LIST_H_

#include <stdio.h>
#include "list.c"

/**
 * @brief Estructura de una lista. 
 */
struct list;

/**
 * @brief Inicializa una lista ya creada.
 * @param l Lista a inicializar.
 */
void initializeList(struct list *l);

/**
 * @brief Agrega el elemento dado a la lista dada.
 * @param element Elemento a agregar.
 * @param l Lista donde se agrega el elemento.
 */
void addElement(char *element, struct list *l);

/**
 * @brief Devuelve el elemento de la primera caja de la lista.
 * @param l Lista de donde se devuelve el elemento.
 * @retval NULL Si la lista esta vacia.
 * @retval char* El elemento de la primera caja.
 */ 
char *getElement(struct list *l);

/**
 * @brief Devuelve el tamano de la lista.
 * @param l La lista.
 * @return Tamano de la lista.
 */
int listSize(struct list *l);

/**
 * @brief Agrega el penultimo respaldo nuevamente a la lista.
 * @param l La lista.
 */
void restoreBackup(struct list *l);

/**
 * @brief Destructor de la lista.
 * @param l La lista a liberar.
 */
void destroyList(struct list *l);

#endif