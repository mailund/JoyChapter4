
#include "chained_hash.h"

#include <stdlib.h>

#include "linked_lists.h"

LIST
get_key_bin(struct hash_table *table, unsigned int key)
{
  unsigned int mask = table->size - 1;
  unsigned int index = key & mask;
  return table->bins + index;
}

struct hash_table *
new_table(unsigned int size)
{
  struct hash_table *table = malloc(sizeof *table);
  table->bins = malloc(size * sizeof *table->bins);
  table->size = size;
  table->used = 0;
  for (LIST bin = table->bins; bin < table->bins + table->size; bin++) {
    *bin = NULL;
  }
  return table;
}

void
free_table(struct hash_table *table)
{
  for (LIST bin = table->bins; bin < table->bins + table->size; bin++) {
    free_list(bin);
  }
  free(table->bins);
  free(table);
}

// This version copies links.
static void
resize(struct hash_table *table, unsigned int new_size)
{
  // remember these...
  unsigned int old_size = table->size;
  struct link **old_bins = table->bins;

  // set up the new table
  table->bins = malloc(new_size * sizeof *table->bins);
  for (LIST bin = table->bins; bin < table->bins + new_size; bin++) {
    *bin = NULL;
  }
  table->size = new_size;

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

void
insert_key(struct hash_table *table, unsigned int key)
{
  LIST bin = get_key_bin(table, key);
  if (!contains_element(bin, key)) { // Avoid duplications
    add_element(bin, key);
    table->used++;
  }
  if (table->used > table->size) {
    resize(table, table->size * 2);
  }
}

bool
contains_key(struct hash_table *table, unsigned int key)
{
  return contains_element(get_key_bin(table, key), key);
}


void
delete_key(struct hash_table *table, unsigned int key)
{
  LIST bin = get_key_bin(table, key);
  if (contains_element(bin, key)) {
    delete_element(bin, key);
    table->used--;
  }
  if (table->used < table->size / 4) {
    resize(table, table->size / 2);
  }
}
