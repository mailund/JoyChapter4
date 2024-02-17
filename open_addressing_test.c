
#include "open_addressing.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static unsigned int
random_key()
{
  unsigned int key = (unsigned int)rand();
  return key;
}

int
main(int argc, const char *argv[])
{
  if (argc < 2) {
    printf("Usage: %s no_elements [load-limit]\n", argv[0]);
    return EXIT_FAILURE;
  }

  double load_limit = 0.5;
  int no_elms = atoi(argv[1]);
  if (argc > 2)
    load_limit = atof(argv[2]);

  unsigned int *keys = malloc(no_elms * sizeof *keys);
  for (int i = 0; i < no_elms; ++i) {
    keys[i] = random_key();
  }

  struct hash_table *table = new_table(2, load_limit);
  clock_t start = clock();
  for (int i = 0; i < no_elms; ++i) {
    printf("Inserting key %u\n", keys[i]);
    insert_key(table, keys[i]);
    print_table(table);
  }
  for (int i = 0; i < no_elms; ++i) {
    printf("Checking that table has key %u\n", keys[i]);
    print_table(table);
    assert(contains_key(table, keys[i]));
  }
  for (int i = 0; i < no_elms; ++i) {
    (void)contains_key(table, random_key());
  }
  for (int i = 0; i < no_elms; ++i) {
    printf("Deleting key %u\n", keys[i]);
    delete_key(table, keys[i]);
    print_table(table);
  }
  for (int i = 0; i < no_elms; ++i) {
    printf("Checking that key %u is no longer there\n", keys[i]);
    print_table(table);
    assert(!contains_key(table, keys[i]));
  }
  clock_t end = clock();
  double elapsed_time = (end - start) / (double)CLOCKS_PER_SEC;
  printf("%g\n", elapsed_time);

  free(keys);
  delete_table(table);

  return EXIT_SUCCESS;
}
