
#include "open_addressing.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

unsigned int
static p(unsigned int k, unsigned int i, unsigned int m)
{
    return (k + i) & (m - 1);
}

static void resize(struct hash_table *table, unsigned int new_size)
{
    // remember the old bins until we have moved them.
    struct bin *old_bins = table->table;
    unsigned int old_size = table->size;

    // Update table so it now contains the new bins (that are empty)
    table->table = malloc(new_size * sizeof *table->table);
    struct bin *end = table->table + new_size;
    for (struct bin *bin = table->table; bin != end; ++bin) {
        bin->is_free = true;
        bin->is_deleted = false;
    }
    table->size = new_size;
    table->active = table->used = 0;

    // the move the values from the old bins to the new, using the table's
    // insertion function
    end = old_bins + old_size;
    for (struct bin *bin = old_bins; bin != end; ++bin) {
        if (bin->is_free || bin->is_deleted) continue;
        insert_key(table, bin->key);
    }

    // finally, free memory for old bins
    free(old_bins);
}

struct hash_table *empty_table(unsigned int size, double load_limit)
{
    struct hash_table *table = malloc(sizeof *table);
    table->table = malloc(size * sizeof *table->table);
    struct bin *end = table->table + size;
    for (struct bin *bin = table->table; bin != end; bin++) {
        bin->is_free = true;
        bin->is_deleted = false;
    }
    table->size = size;
    table->active = table->used = 0;
    table->load_limit = load_limit;
    return table;
}

void delete_table(struct hash_table *table)
{
    free(table->table);
    free(table);
}

void insert_key(struct hash_table *table, unsigned int key)
{
    if (contains_key(table, key)) return;

    unsigned int index;
    for (unsigned int i = 0; i < table->size; ++i) {
        index = p(key, i, table->size);
        struct bin *bin = & table->table[index];

        if (bin->is_free) {
            bin->key = key;
            bin->is_free = bin->is_deleted = false;

            // We have one more active element
            // and one more unused cell changes character
            table->active++; table->used++;
            break;
        }

        if (bin->is_deleted) {
            bin->key = key;
            bin->is_free = bin->is_deleted = false;

            // We have one more active element
            // but we do not use more cells since the
            // deleted cell was already used.
            table->active++;
            break;
        }
    }

    if (table->used > table->load_limit * table->size)
        resize(table, table->size * 2);
}

bool contains_key(struct hash_table *table, unsigned int key)
{
    for (unsigned int i = 0; i < table->size; ++i) {
        unsigned int index = p(key, i, table->size);
        struct bin *bin = & table->table[index];
        if (bin->is_free)
            return false;
        if (!bin->is_deleted && bin->key == key)
            return true;
    }
    return false;
}

void delete_key(struct hash_table *table, unsigned int key)
{
    for (unsigned int i = 0; i < table->size; ++i) {
        unsigned int index = p(key, i, table->size);
        struct bin * bin = & table->table[index];
        if (bin->is_free)
            return;
        if (!bin->is_deleted && bin->key == key) {
            bin->is_deleted = true;
            table->active--;
            break;
        }
    }

    if (table->active < table->load_limit / 4 * table->size)
        resize(table, table->size / 2);
}
