#include "util.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read in pieces, growing the buffer, rather than sized with fseek and
 * ftell: a pipe or a terminal (/dev/stdin) has no size to ask for. */
char *read_entire_file(const char *path) {
    FILE *file = fopen(path, "rb");
    size_t size = 0, capacity = 4096;
    char *buffer;

    if (file == NULL) {
        fprintf(stderr, "cannot open '%s': %s\n", path, strerror(errno));
        exit(74);
    }
    buffer = malloc(capacity);
    for (;;) {
        if (buffer == NULL) {
            fprintf(stderr, "out of memory reading '%s'\n", path);
            exit(70);
        }
        size += fread(buffer + size, 1, capacity - 1 - size, file);
        if (size < capacity - 1)
            break;
        capacity *= 2;
        char *bigger = realloc(buffer, capacity);
        if (bigger == NULL)
            free(buffer);
        buffer = bigger;
    }
    if (ferror(file)) {
        fprintf(stderr, "cannot read '%s': %s\n", path, strerror(errno));
        exit(74);
    }
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}