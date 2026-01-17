#include "../include/utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

char *read_line(FILE *stream) {
    static char buffer[BUFFER_SIZE];
    if (fgets(buffer, BUFFER_SIZE, stream) == NULL) {
        return NULL;
    }
    buffer[strcspn(buffer, "\n")] = '\0';
    return buffer;
}
