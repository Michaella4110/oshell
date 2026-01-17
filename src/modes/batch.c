#include "modes.h"
#include "../include/errors.h"
#include "../include/utils.h"
#include "../include/shell.h"
#include <stdio.h>
#include <stdlib.h>

void batch_mode(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        print_error();
        exit(1);
    }
    
    char *line;
    while ((line = read_line(file)) != NULL) {
        command *cmds = parse_line(line);
        if (cmds) {
            execute_sequence(cmds);
            free_commands(cmds);
        }
    }
    fclose(file);
    exit(0);
}
