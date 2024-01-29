
#ifndef OPEN_ADDRESSING_H
#define OPEN_ADDRESSING_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct bin {
	bool is_free : 1;
	bool is_deleted : 1;
	unsigned int key;
};

struct hash_table {
    struct bin *table;
    unsigned int size;
    unsigned int used;
    unsigned int active;
    double load_limit;
    // only used in primes code, but we share the header, so...
    unsigned int primes_idx;
};

struct hash_table *empty_table(unsigned int size, double load_limit);
void delete_table(struct hash_table *table);

void insert_key  (struct hash_table *table, unsigned int key);
bool contains_key(struct hash_table *table, unsigned int key);
void delete_key  (struct hash_table *table, unsigned int key);

#endif
