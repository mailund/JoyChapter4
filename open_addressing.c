
#include "open_addressing.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

unsigned int static p(unsigned int k, unsigned int i, unsigned int m)
{
  return (k + i) & (m - 1);
}

static void
resize(struct hash_table *table, unsigned int new_size)
{
  // remember the old bins until we have moved them.
  struct bin *old_bins = table->bins;
  unsigned int old_size = table->size;

  // Update table so it now contains the new bins (that are empty)
  table->bins = malloc(new_size * sizeof *table->bins);
  struct bin empty_bin = {.in_probe = false, .is_empty = true};
  for (unsigned int i = 0; i < new_size; i++) {
    table->bins[i] = empty_bin;
  }
  table->size = new_size;
  table->active = table->used = 0;

  // the move the values from the old bins to the new, using the table's
  // insertion function
  for (struct bin *bin = old_bins; bin != old_bins + old_size; bin++) {
    if (bin->in_probe || !bin->is_empty) {
      insert_key(table, bin->key);
    }
  }

  // finally, free memory for old bins
  free(old_bins);
}

struct hash_table *
new_table(unsigned int size, double load_limit)
{
  struct hash_table *table = malloc(sizeof *table);
  table->size = size;
  table->bins = malloc(size * sizeof *table->bins);

  struct bin empty_bin = {.in_probe = false, .is_empty = true};
  for (unsigned int i = 0; i < size; i++) {
    table->bins[i] = empty_bin;
  }

  table->size = size;
  table->active = table->used = 0;
  table->load_limit = load_limit;
  return table;
}

void
delete_table(struct hash_table *table)
{
  free(table->bins);
  free(table);
}

// Find the bin containing key, or the first bin past the end of its probe
struct bin *
find_key(struct hash_table *table, unsigned int key)
{
  for (unsigned int i = 0; i < table->size; i++) {
    struct bin *bin = table->bins + p(key, i, table->size);
    if (bin->key == key || !bin->in_probe)
      return bin;
  }
  // The table is full. We cannot handle that yet!
  assert(false);
}

// Find the bin containing key or the first empty bin in its probe.
struct bin *
find_key_or_empty(struct hash_table *table, unsigned int key)
{
  for (unsigned int i = 0; i < table->size; i++) {
    struct bin *bin = table->bins + p(key, i, table->size);
    if (bin->key == key || bin->is_empty || !bin->in_probe)
      return bin;
  }
  // The table is full. We cannot handle that yet!
  assert(false);
}

void
insert_key(struct hash_table *table, unsigned int key)
{
  struct bin *bin = find_key_or_empty(table, key);
  if (bin->key == key)
    return; // Nothing further to do
  if (!bin->in_probe) {
    // We now will use one more bin
    table->used++;
  }
  table->active++; // We definitely have one more active
  *bin = (struct bin){.in_probe = true, .is_empty = false, .key = key};

  if (table->used > table->load_limit * table->size)
    resize(table, table->size * 2);
}

bool
contains_key(struct hash_table *table, unsigned int key)
{
  struct bin *bin = find_key(table, key);
  return bin->key == key && !bin->is_empty;
}

void
delete_key(struct hash_table *table, unsigned int key)
{
  // This will do nothing if we are at the end of the probe but delete if we are
  // inside a probe
  struct bin *bin = find_key(table, key);
  if (bin->key != key)
    return;             // Nothing more to do
  bin->is_empty = true; // Delete the bin
  if (table->active < table->load_limit / 4 * table->size)
    resize(table, table->size / 2);
}

void
print_table(struct hash_table *table)
{
  for (unsigned int i = 0; i < table->size; i++) {
    if (i > 0 && i % 8 == 0) {
      printf("\n");
    }
    struct bin *bin = table->bins + i;
    if (bin->in_probe && !bin->is_empty) {
      printf("[%u]", bin->key);
    } else if (bin->in_probe && bin->is_empty) {
      printf("[*]");
    } else {
      printf("[ ]");
    }
  }
  printf("\n----------------------\n");
}
