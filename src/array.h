
#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define ARRAY_INIT(array)                                                      \
  do {                                                                         \
    (array)->items = NULL;                                                     \
    (array)->count = 0;                                                        \
    (array)->capacity = 0;                                                     \
  } while (0)

#define ARRAY_FREE(array)                                                      \
  do {                                                                         \
    free((array)->items);                                                      \
    (array)->items = NULL;                                                     \
    (array)->count = 0;                                                        \
    (array)->capacity = 0;                                                     \
  } while (0)

#define ARRAY_GROW_CAPACITY(capacity)                                          \
    ((capacity) < 8 ? 8 :                                                      \
    (capacity) > SIZE_MAX / 2 ? SIZE_MAX : (capacity) * 2)

#define ARRAY_RESERVE(array, new_capacity)                                    \
  do {                                                                        \
    size_t capacity = (new_capacity);                                         \
                                                                              \
    if (capacity > SIZE_MAX / sizeof(*(array)->items)) {                      \
      fprintf(stderr, "array: capacity overflow\n");                          \
      exit(70);                                                               \
    }                                                                         \
                                                                              \
    if (capacity > (array)->capacity) {                                       \
      void* new_items =                                                       \
          realloc((array)->items, capacity * sizeof(*(array)->items));        \
                                                                              \
      if (new_items == NULL) {                                                \
        fprintf(stderr, "out of memory\n");                                   \
        exit(70);                                                             \
      }                                                                       \
                                                                              \
      (array)->items = new_items;                                             \
      (array)->capacity = capacity;                                           \
    }                                                                         \
  } while (0)

#define ARRAY_PUSH(array, value)                                               \
  do {                                                                         \
    if ((array)->count >= (array)->capacity) {                                 \
      size_t new_capacity = ARRAY_GROW_CAPACITY((array)->capacity);            \
                                                                               \
      ARRAY_RESERVE((array), new_capacity);                                    \
    }                                                                          \
                                                                               \
    (array)->items[(array)->count++] = (value);                                \
  } while (0)

#define ARRAY_POP(array) ((array)->items[--(array)->count])

#define ARRAY_LAST(array) ((array)->items[(array)->count - 1])

#define ARRAY_CLEAR(array) ((array)->count = 0)

/*
 * Iterates over all elements in a dynamic array using an index.
 *
 * Parameters:
 *   array - Pointer to the dynamic array.
 *   index - Name of the loop index variable (size_t).
 *
 * Example:
 *   ARRAY_FOREACH(&numbers, i) {
 *       printf("%d\n", numbers.items[i]);
 *   }
 *
 * Note: Avoid adding or removing elements during iteration.
 */
#define ARRAY_FOREACH(array, index)                                            \
  for (size_t index = 0; index < (array)->count; index++)

#endif
