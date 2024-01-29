
#ifndef LINKED_LISTS_H
#define LINKED_LISTS_H

#include <stdbool.h>
#include <stdlib.h>

struct link {
  unsigned int key;
  struct link *next;
};
typedef struct link *LIST_HEAD;
typedef LIST_HEAD *LIST;

void init_linked_list(LIST list);
void delete_linked_list(LIST list);

void add_element(LIST list, unsigned int key);
void delete_element(LIST list, unsigned int key);
bool contains_element(LIST list, unsigned int key);

#endif
