#ifndef LIST_
#define LIST_

#include <stdlib.h>
#include <stdio.h>
#define SIZE 1000

struct list {

   struct box *head;
   struct box *last;
   struct box *backup;
   int size;
};

struct box {

   char *element;
   struct box *next;
};

void initializeList(struct list *l) {

   l->head = NULL;
   l->last = NULL;
   l->backup = NULL;
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

   char *elem = elemBox->element;

   free(elemBox);

   return elem;
}

int listSize(struct list *l) {

   return l->size;
}
/*
int main () {

   struct list l;
   initializeList(&l);

   addElement("hola",&l);

   char *elem = getElement(&l);

   addElement("hola2",&l);

   elem = getElement(&l);

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