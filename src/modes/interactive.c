#include "modes.h"
#include "../include/utils.h"
#include "../include/shell.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static void handle_sigint(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\n$ ", 3);
}

void interactive_mode(void) {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    
    char *line;
    while (1) {
        printf("$ ");
        fflush(stdout);
        line = read_line(stdin);
        if (!line) {
            printf("\n");
            break;
        }
        command *cmds = parse_line(line);
        if (cmds) {
            execute_sequence(cmds);
            free_commands(cmds);
        }
    }
}
