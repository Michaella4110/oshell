#include "../include/shell.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

shell_state g_state;

void init_shell_state(void) {
    // Default path: /bin
    g_state.path_count = 1;
    g_state.path_list = malloc(2 * sizeof(char *));
    g_state.path_list[0] = strdup("/bin");
    g_state.path_list[1] = NULL;

    // PWD and OLDPWD
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        g_state.pwd = strdup(cwd);
        g_state.oldpwd = strdup(cwd);
    } else {
        g_state.pwd = strdup("/");
        g_state.oldpwd = strdup("/");
    }

    g_state.exit_status = 0;
    g_state.shell_pid = getpid();
}

void free_shell_state(void) {
    for (int i = 0; i < g_state.path_count; i++) {
        free(g_state.path_list[i]);
    }
    free(g_state.path_list);
    free(g_state.pwd);
    free(g_state.oldpwd);
}
