
#include "linked_lists.h"

#include <assert.h>
#include <stdio.h>

void init_linked_list(LIST list) {
  assert(list);
  *list = NULL;
}

struct link *new_link(unsigned int key, struct link *next) {
  struct link *link = malloc(sizeof *link);
  *link = (struct link){.key = key, .next = next};
  return link;
}

void delete_linked_list(LIST list) {
  while (*list) {
    struct link *next = (*list)->next;
    free(*list);
    *list = next;
  }
}

void add_element(LIST list, unsigned int key) {
  // Build link and put it at the front of the list.
  // The hash table checks for duplicates if we want to
  // avoid those
  *list = new_link(key, *list);
}

// Get the reference of (pointer to) the pointer to the link that
// contains the key. That is, get a pointer to the 'next' pointer
// in the previous link (if there is one) or a pointer to the head
// of the list. From that pointer, we can access both the link
// that holds the key, and also update the pointer to the link that
// holds the link to delete the link.
struct link **ref_to_link(LIST list, unsigned int key) {
  for (struct link **ref = list; *ref; ref = &(*ref)->next) {
    if ((*ref)->key == key) return ref;
  }
  return NULL;
}

void delete_element(LIST list, unsigned int key) {
  struct link **prev_ref = ref_to_link(list, key);
  if (prev_ref) {
    // Cut out *prev_ref as this is the link that contains
    // key. First, save its next, then we can free it, and
    // finally update the list by updating the pointer
    // that previously pointed to the now deleted link.
    struct link *next = (*prev_ref)->next;
    free(*prev_ref);
    *prev_ref = next;
  }
}

bool contains_element(LIST list, unsigned int key) {
  return ref_to_link(list, key) != 0;
}
