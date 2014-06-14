#include <stdio.h>


struct list;
void initializeList(struct list *l);
void addElement(char *element, struct list *l);
char *deleteElement(struct list *l);
int listSize(struct list *l);