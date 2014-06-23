#ifndef LIST_
#define LIST_

#include <stdlib.h>
#include <stdio.h>
#define SIZE 1000

struct list {

   struct box *head;
   struct box *last;
   struct box *secondLastBackup;
   struct box *lastBackup;
   int size;
};

struct box {

   char *element;
   struct box *next;
};

void initializeList(struct list *l) {

   l->head = NULL;
   l->last = NULL;
   l->secondLastBackup = NULL;
   l->lastBackup = NULL;
   l->size = 0;
}

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

char *getElement(struct list *l) {

   if (l->head == NULL)
      return NULL;

   struct box *elemBox = l->head;
   l->head = l->head->next;
   --(l->size);

   if (l->size == 0)
      l->last = NULL;

   char *elem = elemBox->element;

   if (l->secondLastBackup != NULL && l->secondLastBackup != elemBox)
      free(l->secondLastBackup);


   l->secondLastBackup = l->lastBackup;
   l->lastBackup = elemBox;

   return elem;
}

int listSize(struct list *l) {

   return l->size;
}

void restoreBackup(struct list *l) {

   if (l->secondLastBackup != NULL) {

      l->secondLastBackup->next = l->head;
      l->head = l->secondLastBackup;
      ++(l->size);

      if (l->size == 1)
         l->last = l->head;
   }
}



/*
int main () {

   struct list l;
   initializeList(&l);

   addElement("hola",&l);

   char *elem = getElement(&l);

   restoreBackup(&l);

   addElement("hola2",&l);

   elem = getElement(&l);

   restoreBackup(&l);

   addElement("hola3",&l);
   addElement("hola4",&l);
   addElement("hola5",&l);

   while (listSize(&l) > 0) {

      elem = getElement(&l);
      if (elem != NULL)
         printf("elem: %s\n", elem);
   }
   return 0;
}*/

#endif