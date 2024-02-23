
#include "dynamic_chained_hash.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "linked_lists.h"

static const unsigned int SUBTABLE_BITS = 3; // 8 bins to a sub-table

// A sub-table is an array of pointers to links.
// A sub-table plus an index is also a struct link **
// which by good fortune is a LIST.
typedef struct link **subtable;
struct hash_table {
  subtable *tables;        // Tables is an array of sub-tables
  unsigned int m;          // Number of bins in the buttom half of the table
  unsigned int max_init;   // Max index of initialised bins.
  unsigned int table_bits; // Bits used for indexing into sub-tables
  unsigned int split;      // Pointer to the bin we need to split/merge
};

// Size of a word with `bits` bits
static inline unsigned int
bits_size(unsigned int bits)
{
  return 1 << bits;
}
// Mask for the lower `bits` bits
static inline unsigned int
bit_mask(unsigned int bits)
{
  return bits_size(bits) - 1;
}

// A mask that matches the current bit we are splitting on
static inline unsigned int
split_bit_mask(struct hash_table *table)
{
  return 1 << (table->table_bits + SUBTABLE_BITS);
}
// The largest bin that is currently in use
static inline unsigned int
max_index(struct hash_table *table)
{
  return table->m + table->split;
}
// A mask for the parts of hash keys we are currently considering
static inline unsigned int
key_mask(struct hash_table *table)
{
  return bit_mask(table->table_bits + 1 + SUBTABLE_BITS);
}

// The bins up to split + m are valid, the higher indices are not.
// If we are below this index, we can use the index, otherwise we need
// to use one table bit less.
static inline unsigned int
key_in_table_range(struct hash_table *table, unsigned int hash_key)
{
  unsigned int masked_key = hash_key & key_mask(table);
  return (masked_key < max_index(table))
             ? masked_key
             : (masked_key & ~split_bit_mask(table));
}
// Indexing
// With compiler optimisation, this struct is likely to have the same size as an
// unsigned int and splitting the bits into indices is simply a cast. Checked
// with gcc, zig, icx, clang but it does not seem to happen with icc and msvc.
// We still need to split the index, though, so there is no way around it.
// Adding the masking complicates it slightly, but is necessary after all.
struct index {
  unsigned int bin : SUBTABLE_BITS;
  unsigned int table : sizeof(unsigned int) * CHAR_BIT - SUBTABLE_BITS;
};

static inline struct index
key_index(unsigned int hash_key)
{
  return (struct index){.bin = hash_key & bit_mask(SUBTABLE_BITS),
                        .table = (hash_key >> SUBTABLE_BITS)};
}
static inline struct index
index_in_table_range(struct hash_table *table, unsigned int hash_key)
{
  return key_index(key_in_table_range(table, hash_key));
}

// Get a bin from an index
static inline LIST
get_bin(struct hash_table *table, struct index idx)
{
  return &table->tables[idx.table][idx.bin];
}

struct hash_table *
new_table()
{
  struct hash_table *table = malloc(sizeof *table);
  // Allocate the table of tables and the first two tables.
  table->tables = malloc(2 * sizeof *table->tables);

  // Initialise the first table only
  table->tables[0] =
      malloc(bits_size(SUBTABLE_BITS) * sizeof *table->tables[0]);
  for (unsigned int i = 0; i < bits_size(SUBTABLE_BITS); i++) {
    table->tables[0][i] = NULL;
  }

  table->m =
      bits_size(SUBTABLE_BITS);   // We have one full sub-table to begin with
  table->max_init = table->m - 1; // Max index of initialised bins
  table->table_bits = 0;          // we only use bin bits initially
  table->split = 0;               // we start splitting at the first bin

  return table;
}

void
delete_table(struct hash_table *table)
{
  // Delete lists in all initialised bins
  for (unsigned int bin = 0; bin < table->max_init; bin++) {
    free_list(get_bin(table, key_index(bin)));
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

static void
grow_tables(struct hash_table *table)
{
  // Grow table if we have inserted m elements.
  if (table->split == table->m) {
    // Use one more bit for table indices
    table->table_bits++;
    table->m *= 2;

    // Alloc more table pointers (but don't initialise, we do that
    // incrementally)
    size_t new_size = 2 * bits_size(table->table_bits) * sizeof *table->tables;
    table->tables = realloc(table->tables, new_size);

    // Reset split pointer
    table->split = 0;
  }
}

void
split_bin(LIST from_bin, LIST to_bin, unsigned int split_bit)
{
  struct link *link = *from_bin; // Catch list before we clear the bin.
  *from_bin = NULL;              // Make bin ready for new values

  while (link) {
    struct link *next = link->next;
    if (link->key & split_bit) {
      // Move link
      link->next = *to_bin;
      *to_bin = link;
    } else {
      // Put link back into its current bin
      link->next = *from_bin;
      *from_bin = link;
    }
    link = next;
  }
}

static void
split(struct hash_table *table)
{
  // Initialise the target bin at split + m.
  struct index to_idx = key_index(max_index(table));
  if (to_idx.bin == 0 && table->split + table->m == table->max_init + 1) {
    // If we are moving into a new sub-table, we need to allocate it
    table->tables[to_idx.table] =
        malloc(bits_size(SUBTABLE_BITS) * sizeof *table->tables[to_idx.table]);
  }
  LIST to_bin = get_bin(table, to_idx);
  *to_bin = NULL;

  // Get the split bin and if there are elements there, split them.
  LIST from_bin = get_bin(table, key_index(table->split));
  split_bin(from_bin, to_bin, split_bit_mask(table));

  // Update counters to reflect that we have split
  if (table->split + table->m > table->max_init)
    table->max_init = table->split + table->m;
  table->split++;
}

static void
grow(struct hash_table *table)
{
  grow_tables(table);
  split(table);
}

void
insert_key(struct hash_table *table, unsigned int key)
{
  LIST bin = get_bin(table, index_in_table_range(table, key));
  if (!contains_element(bin, key)) {
    add_element(bin, key);
    grow(table);
  }
}

bool
contains_key(struct hash_table *table, unsigned int key)
{
  LIST bin = get_bin(table, index_in_table_range(table, key));
  return contains_element(bin, key);
}

static void
merge_bins(LIST from_bin, LIST to_bin)
{
  struct link *link = *from_bin;
  while (link) {
    struct link *next = link->next;
    link->next = *to_bin;
    *to_bin = link;
    link = next;
  }
}

static void
shrink_tables(struct hash_table *table)
{
  // If we have reduced the size from 8m to 2m, reduce the table size to 4m.
  if (table->max_init > 8 * table->m) {
    unsigned int from_index = (4 * table->m) >> SUBTABLE_BITS;
    unsigned int tables_allocated = table->max_init >> SUBTABLE_BITS;
    // Free the sub-tables that are now lost.
    for (unsigned int i = from_index; i < tables_allocated; i++) {
      free(table->tables[i]);
    }
    // Reduce the size of tables.
    size_t new_size = from_index * sizeof *table->tables;
    table->tables = realloc(table->tables, new_size);
    // And now the max initialised is 4m.
    table->max_init = 4 * table->m;
  }
}

// Decrement split. If it is a zero, we need to
// decrement table_bits and m instead, and set split to m - 1.
static inline void
dec_split(struct hash_table *table)
{
  if (table->split > 0) {
    table->split--;
  } else {
    table->table_bits--;
    table->m /= 2;
    table->split = table->m - 1;
  }
}

static void
shrink(struct hash_table *table)
{
  // Decrement split. If it is a zero, we need to
  // decrement table_bits and m instead, and set split to m - 1.
  dec_split(table);

  // Merge largest bin into split bin (well, one before the split bin so the
  // indices match)
  merge_bins(get_bin(table, key_index(max_index(table))),
             get_bin(table, key_index(table->split)));

  shrink_tables(table);
}

void
delete_key(struct hash_table *table, unsigned int key)
{
  LIST bin = get_bin(table, index_in_table_range(table, key));
  if (contains_element(bin, key)) {
    delete_element(bin, key);
    shrink(table);
  }
}

void
print_table(struct hash_table *table)
{
  unsigned int slot;
  for (slot = 0; slot < max_index(table); slot++) {
    if ((slot & bit_mask(SUBTABLE_BITS)) == 0)
      printf("\n");
    LIST bin = get_bin(table, key_index(slot));
    char *sep = "";
    if (slot == table->split)
      printf("->");
    printf("[");
    struct link *x;
    for (x = *bin; x; x = x->next) {
      printf("%s%u", sep, x->key);
      sep = "|";
    }
    printf("]");
  }
  printf("\n");
  printf("Usage is %u\n", max_index(table));
}
