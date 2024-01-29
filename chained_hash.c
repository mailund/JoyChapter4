
#include "chained_hash.h"

#include <stdio.h>
#include <stdlib.h>

#include "linked_lists.h"

LIST get_key_bin(struct hash_table *table, unsigned int key) {
  // table->table is an array of LIST_HEAD i.e. an array of struct link *,
  // which makes table->table plus any offset a struct link **, i.e., a LIST.
  unsigned int mask = table->size - 1;
  unsigned int index = key & mask;
  return table->bins + index;
}

struct hash_table *new_table(unsigned int size) {
  struct hash_table *table = malloc(sizeof *table);
  table->bins = malloc(size * sizeof *table->bins);
  table->size = size;
  table->used = 0;
  for (LIST bin = table->bins; bin < table->bins + table->size; bin++) {
    init_linked_list(bin);
  }
  return table;
}

void delete_table(struct hash_table *table) {
  for (LIST bin = table->bins; bin < table->bins + table->size; bin++) {
    delete_linked_list(bin);
  }
  free(table->bins);
  free(table);
}

#if 0
// Using insert and delete to move keys
static void resize(struct hash_table *table, unsigned int new_size) {
  // Remember these...
  unsigned int old_size = table->size;
  LIST_HEAD *old_bins = table->bins;

  // set up the new table
  table->bins = malloc(new_size * sizeof *table->bins);
  for (size_t i = 0; i < new_size; ++i) {
    init_linked_list(get_bin(table, i));
  }
  table->size = new_size;
  table->used = 0;  // set to zero as we increment when inserting

  // Copy keys
  for (LIST old_bin = old_bins; old_bin < old_bins + old_size; old_bin++) {
    for (struct link *link = *old_bin; link; link = link->next) {
      insert_key(table, link->key);
    }
  }

  // Delete old table
  for (LIST old_bin = old_bins; old_bin < old_bins + old_size; old_bin++) {
    delete_linked_list(old_bin);
  }
  free(old_bins);
}

#else

// This version copies links.
static void resize(struct hash_table *table, size_t new_size) {
  // remember these...
  unsigned int old_size = table->size;
  LIST_HEAD *old_bins = table->bins;

  // set up the new table
  table->bins = malloc(new_size * sizeof *table->bins);
  for (size_t i = 0; i < new_size; ++i) {
    init_linked_list(get_key_bin(table, i));
  }
  table->size = new_size;
  // table->used remains the same since we don't use insert_key
  // to add elements, we simply move the links.

  // copy keys
  for (LIST old_bin = old_bins; old_bin < old_bins + old_size; old_bin++) {
    while (*old_bin) {
      struct link *link = *old_bin;
      // remove first link from old bin
      *old_bin = link->next;
      // connect link to the new bin
      LIST new_bin = get_key_bin(table, link->key);
      link->next = *new_bin;
      *new_bin = link;
    }
  }

  free(old_bins);
}
#endif

void insert_key(struct hash_table *table, unsigned int key) {
  LIST bin = get_key_bin(table, key);
  if (!contains_element(bin, key)) {  // Avoid duplications
    add_element(bin, key);
    table->used++;
  }
  if (table->used > table->size / 2) {
    resize(table, table->size * 2);
  }
}

bool contains_key(struct hash_table *table, unsigned int key) {
  return contains_element(get_key_bin(table, key), key);
}

void delete_key(struct hash_table *table, unsigned int key) {
  LIST bin = get_key_bin(table, key);
  if (contains_element(bin, key)) {
    delete_element(bin, key);
    table->used--;
  }
  if (table->used < table->size / 8) {
    resize(table, table->size / 2);
  }
}
