#ifndef LIST_H_
#define LIST_H_

#include <stdio.h>
#include "list.c"

struct list;
void initializeList(struct list *l);
void addElement(char *element, struct list *l);
char *getElement(struct list *l);
int listSize(struct list *l);


#endif