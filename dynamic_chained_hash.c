
#include "dynamic_chained_hash.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "linked_lists.h"

#define MAX(A, B) ((A < B) ? (B) : (A))

#define SIZE(BITS) (1 << (BITS))     // Size of a table indexed with BITS bits
#define MASK(BITS) (SIZE(BITS) - 1)  // Mask for a table index with BITS bits
#define SUBTABLE_BITS 3              // 8 bins to a sub-table

struct hash_table {
  LIST_HEAD **tables;  // tables is an array of pointers to sub-tables

  unsigned int m;           // Number of bins in the buttom half of the table
  unsigned int max_init;    // Max index of initialised bins.
  unsigned int table_bits;  // bits used for indexing into sub-tables
  unsigned int split;       // pointer to the bin we need to split/merge
};

// Indexing
struct idx {
  unsigned int table;
  unsigned int bin;
};

// With compiler optimisation, this struct is likely to have the same size as an
// unsigned int and splitting the bits into indices is simply a cast. Checked
// with gcc, zig, icx, clang but it does not seem to happen with icc and msvc.
// We still need to split the index, though, so there is no way around it.
// Adding the masking complicates it slightly, but is necessary after all.
struct index {
  unsigned int bin : SUBTABLE_BITS;
  unsigned int table : sizeof(unsigned int) * CHAR_BIT - SUBTABLE_BITS;
};

struct index split_key(unsigned int hash_key) {
  return (struct index){.bin = hash_key & MASK(SUBTABLE_BITS),
                        .table = (hash_key >> SUBTABLE_BITS)};
}
unsigned int merge_key(struct index idx) {
  return idx.table << SUBTABLE_BITS | idx.bin;
}

// The bins up to split + m are valid, the higher indices are not.
// If we are below this index, we can use the index, otherwise we need
// to use one table bit less.
struct index valid_index(struct hash_table *table, unsigned int hash_key) {
  unsigned int masked_key =
      hash_key & MASK(table->table_bits + 1 + SUBTABLE_BITS);
  if (masked_key >= table->split + table->m) {
    masked_key &= MASK(table->table_bits + SUBTABLE_BITS);
  }
  return split_key(masked_key);
}

LIST get_index_bin(struct hash_table *table, struct index idx) {
  return &table->tables[idx.table][idx.bin];
}
LIST get_key_bin(struct hash_table *table, unsigned int hash_key) {
  return get_index_bin(table, valid_index(table, hash_key));
}

struct hash_table *new_table() {
  struct hash_table *table = malloc(sizeof *table);
  // Allocate the table of tables and the first two tables.
  table->tables = malloc(2 * sizeof *table->tables);

  // Initialise the first table only
  table->tables[0] = malloc(SIZE(SUBTABLE_BITS) * sizeof *table->tables[0]);
  for (unsigned int i = 0; i < SIZE(SUBTABLE_BITS); i++) {
    init_linked_list(&table->tables[0][i]);
  }

  table->m = SIZE(SUBTABLE_BITS);  // We have one full sub-table to begin with
  table->max_init = table->m - 1;  // Max index of initialised bins
  table->table_bits = 0;           // we only use bin bits initially
  table->split = 0;                // we start splitting at the first bin

  return table;
}

void delete_table(struct hash_table *table) {
  // Delete lists in all initialised bins
  for (unsigned int bin = 0; bin < table->max_init; bin++) {
    delete_linked_list(get_key_bin(table, bin));
  }

  // Then free all sub-tables
  unsigned int tables_allocated = table->max_init >> SUBTABLE_BITS;
  for (unsigned int i = 0; i < tables_allocated; i++) {
    free(table->tables[i]);
  }

  // And finally free the tables array and the table
  free(table->tables);
  free(table);
}

static void grow_tables(struct hash_table *table) {
  // Grow table if we have inserted m elements.
  if (table->split == table->m) {
    // Use one more bit for table indices
    table->table_bits++;
    table->m *= 2;

    // Alloc more table pointers (but don't initialise, we do that
    // incrementally)
    size_t new_size = 2 * SIZE(table->table_bits) * sizeof *table->tables;
    table->tables = realloc(table->tables, new_size);

    // Reset split pointer
    table->split = 0;
  }
}

static void split(struct hash_table *table) {
  struct index from_idx = split_key(table->split);
  struct index to_idx = split_key(table->split + table->m);

  if (to_idx.bin == 0 && merge_key(to_idx) == table->max_init + 1) {
    table->tables[to_idx.table] =
        malloc(SIZE(SUBTABLE_BITS) * sizeof *table->tables[to_idx.table]);
  }

  LIST from_bin = get_index_bin(table, from_idx);
  LIST to_bin = get_index_bin(table, to_idx);

  // Catch the front link of the from_bin list and reset the two lists to make
  // them ready for the split.
  struct link *link = *from_bin;
  init_linked_list(from_bin);
  init_linked_list(to_bin);

  // Now split the from_bin into two based on the left-most bit in the full key
  // width
  unsigned int split_bit = 1 << (table->table_bits + SUBTABLE_BITS);
  while (link) {
    struct link *next = link->next;
    if (link->key & split_bit) {
      // Move link
      link->next = *to_bin;
      *to_bin = link;
    } else {
      // Put link back in its current bin
      link->next = *from_bin;
      *from_bin = link;
    }
    link = next;
  }

  // Update counters to reflect that we have split
  table->split++;
  table->max_init = MAX(merge_key(to_idx), table->max_init);
}

static void grow(struct hash_table *table) {
  grow_tables(table);
  split(table);
}

static void shrink(struct hash_table *table) {
#warning Not implemented yet
}

void insert_key(struct hash_table *table, unsigned int key) {
  grow(table);
  LIST bin = get_key_bin(table, key);
  if (!contains_element(bin, key)) {
    add_element(bin, key);
  }
}

bool contains_key(struct hash_table *table, unsigned int key) {
  LIST bin = get_key_bin(table, key);
  return contains_element(bin, key);
}

void delete_key(struct hash_table *table, unsigned int key) {
  LIST bin = get_key_bin(table, key);
  if (contains_element(bin, key)) {
    delete_element(bin, key);
    shrink(table);
  }
}

void print_table(struct hash_table *table) {
  for (unsigned int slot = 0; slot <= table->max_init; slot++) {
    if ((slot & MASK(SUBTABLE_BITS)) == 0) printf("\n");
    LIST bin = get_key_bin(table, slot);
    char *sep = "";
    if (slot == table->split) printf("->");
    printf("[");
    struct link *x;
    for (x = *bin; x; x = x->next) {
      printf("%s%u", sep, x->key);
      sep = "|";
    }
    printf("]");
  }
  printf("\n");
}
