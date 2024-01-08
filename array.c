
#include "array.h"

#include <assert.h>
#include <stdlib.h>

struct array *new_array(int initial_size) {
  struct array *array = malloc(sizeof *array);
  *array = (struct array){.size = initial_size,
                          .used = 0,
                          .array = malloc(initial_size * sizeof *array->array)};
  return array;
}

void delete_array(struct array *array) {
  free(array->array);
  free(array);
}

static void resize(struct array *array, unsigned int new_size) {
  assert(new_size >= array->used);
  array->array = realloc(array->array, new_size * sizeof *array->array);
}

void append(struct array *array, int value) {
  if (array->used == array->size) resize(array, 2 * array->size);
  array->array[array->used++] = value;
}

int get(struct array *array, unsigned int index) {
  assert(array->used > index);
  return array->array[index];
}

void set(struct array *array, unsigned int index, int value) {
  assert(array->used > index);
  array->array[index] = value;
}

int pop(struct array *array) {
  assert(array->used > 0);
  int top = array->array[--(array->used)];
  if (array->used < array->size / 4) resize(array, array->size / 2);
  return top;
}
