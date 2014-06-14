#ifndef LIST_
#define LIST_

#include <stdlib.h>
#define SIZE 1000

struct list {

   char *elements[SIZE];
   int lower;
   int upper;
   int isFull;
};

void initializeList(struct list *l) {

   l->lower = 0;
   l->upper = 0;
   l->isFull = 0;

}

void addElement(char *element, struct list *l) {

   if (!l->isFull) {

      l->elements[l->upper] = element;
      l->upper = (l->upper+1) % SIZE;
   }

   if (l->lower == l->upper)
      l->isFull = 1;
}

char *getElement(struct list *l) {

   if (l->lower == l->upper && !l->isFull) {

      return NULL;

   } else {

      char *elem = l->elements[l->lower];

      l->lower = (l->lower+1) % SIZE;

      if (l->isFull)
         l->isFull = 0;

      return elem;
   }
}

int listSize(struct list *l) {

   int size;

   if (l->lower < l->upper)
      size = l->upper - l->lower;

   else if (l->lower == l->upper && !l->isFull)
      size = 0;
   else if (l->lower == l->upper && l->isFull)
      size = SIZE;
   else
      size = SIZE - (l->lower - l->upper);

   return size;


}
/*
int main () {

   struct list l;
   initializeList(&l);

   addElement("hola",&l);
   char *elem = deleteElement(&l);

   printf("elem: %s\n", elem);
   return 0;
}*/

#endif