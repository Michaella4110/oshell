#include "modes.h"
#include "../include/utils.h"
#include "../include/shell.h"
#include <stdio.h>

void pipe_mode(void) {
    char *line;
    while ((line = read_line(stdin)) != NULL) {
        command *cmds = parse_line(line);
        if (cmds) {
            execute_sequence(cmds);
            free_commands(cmds);
        }
    }
}
