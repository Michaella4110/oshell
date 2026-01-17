#include "../include/shell.h"
#include "../include/errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

extern shell_state g_state;

// ============= ALIAS SYSTEM =============
typedef struct alias {
    char *name;
    char *value;
    struct alias *next;
} alias;

static alias *alias_list = NULL;

static void alias_add(const char *name, const char *value) {
    alias *a = alias_list;
    while (a) {
        if (strcmp(a->name, name) == 0) {
            free(a->value);
            a->value = strdup(value);
            return;
        }
        a = a->next;
    }
    
    alias *new = malloc(sizeof(alias));
    new->name = strdup(name);
    new->value = strdup(value);
    new->next = alias_list;
    alias_list = new;
}

char *expand_alias(const char *name) {
    alias *a = alias_list;
    while (a) {
        if (strcmp(a->name, name) == 0) {
            return a->value;
        }
        a = a->next;
    }
    return NULL;
}

static void alias_print_one(const char *name) {
    char *val = expand_alias(name);
    if (val) {
        printf("%s='%s'\n", name, val);
    }
}

static void alias_print_all(void) {
    alias *a = alias_list;
    while (a) {
        printf("%s='%s'\n", a->name, a->value);
        a = a->next;
    }
}

static void free_aliases(void) {
    alias *a = alias_list;
    while (a) {
        alias *next = a->next;
        free(a->name);
        free(a->value);
        free(a);
        a = next;
    }
    alias_list = NULL;
}

// ============= BUILTIN COMMANDS =============
static int builtin_exit(char **args) {
    if (args[1] == NULL) {
        free_aliases();
        exit(0);
    }
    
    char *endptr;
    long val = strtol(args[1], &endptr, 10);
    
    if (*endptr != '\0' || endptr == args[1]) {
        print_error();
        return 1;
    }
    
    int status = (int)(val & 0xFF);
    free_aliases();
    exit(status);
    return 0;
}

static int builtin_cd(char **args) {
    char *target = NULL;
    char *old = g_state.pwd;
    int should_print = 0;
    
    if (args[1] == NULL) {
        target = getenv("HOME");
        if (target == NULL) {
            print_error();
            return 1;
        }
    } else if (strcmp(args[1], "-") == 0) {
        target = g_state.oldpwd;
        should_print = 1;
        if (target == NULL) {
            print_error();
            return 1;
        }
    } else if (strcmp(args[1], "--") == 0) {
        target = g_state.oldpwd;
        if (target == NULL) {
            print_error();
            return 1;
        }
    } else {
        target = args[1];
    }

    if (chdir(target) != 0) {
        print_error();
        return 1;
    }

    free(g_state.oldpwd);
    g_state.oldpwd = strdup(old);
    
    free(g_state.pwd);
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        print_error();
        return 1;
    }
    g_state.pwd = strdup(cwd);
    
    if (should_print) {
        printf("%s\n", g_state.pwd);
    }
    
    return 0;
}

static int builtin_env(char **args) {
    (void)args;
    extern char **environ;
    for (char **env = environ; *env != NULL; env++) {
        printf("%s\n", *env);
    }
    return 0;
}

static int builtin_setenv(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        print_error();
        return 1;
    }
    if (setenv(args[1], args[2], 1) != 0) {
        print_error();
        return 1;
    }
    return 0;
}

static int builtin_unsetenv(char **args) {
    if (args[1] == NULL) {
        print_error();
        return 1;
    }
    unsetenv(args[1]);
    return 0;
}

static int builtin_alias(char **args) {
    if (args[1] == NULL) {
        alias_print_all();
        return 0;
    }
    
    for (int i = 1; args[i]; i++) {
        char *eq = strchr(args[i], '=');
        if (eq) {
            *eq = '\0';
            char *name = args[i];
            char *value = eq + 1;

            size_t len = strlen(value);
            if (len >= 2 && value[0] == '\'' && value[len-1] == '\'') {
                char *unquoted = malloc(len - 1);
                strncpy(unquoted, value + 1, len - 2);
                unquoted[len - 2] = '\0';
                alias_add(name, unquoted);
                free(unquoted);
            } else {
                alias_add(name, value);
            }
        } else {
            alias_print_one(args[i]);
        }
    }
    return 0;
}

static int builtin_path(char **args) {
    for (int i = 0; i < g_state.path_count; i++) {
        free(g_state.path_list[i]);
    }
    free(g_state.path_list);

    int new_count = 0;
    for (int i = 1; args[i]; i++) {
        new_count++;
    }

    g_state.path_count = new_count;
    if (new_count == 0) {
        g_state.path_list = NULL;
    } else {
        g_state.path_list = malloc((new_count + 1) * sizeof(char *));
        for (int i = 0; i < new_count; i++) {
            g_state.path_list[i] = strdup(args[i + 1]);
        }
        g_state.path_list[new_count] = NULL;
    }

    return 0;
}

// ============= MAN BUILTIN =============
static int builtin_man(char **args) {
    if (args[1] == NULL) {
        printf("Usage: man [command]\n");
        printf("Available commands: exit, cd, env, setenv, unsetenv, alias, path, oshell, builtins\n");
        return 0;
    }
    
    char *manpage = args[1];
    char *manpages[] = {
        "exit", "cd", "env", "setenv", "unsetenv", 
        "alias", "path", "oshell", "builtins", NULL
    };
    
    // Check if valid man page
    int valid = 0;
    for (int i = 0; manpages[i]; i++) {
        if (strcmp(manpage, manpages[i]) == 0) {
            valid = 1;
            break;
        }
    }
    
    if (!valid) {
        printf("No manual entry for '%s'\n", manpage);
        printf("Available: exit, cd, env, setenv, unsetenv, alias, path, oshell, builtins\n");
        return 1;
    }
    
    // Construct path to man page
    char path[256];
    snprintf(path, sizeof(path), "man/%s.1", manpage);
    
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        // Try from parent directory (if running from src/)
        snprintf(path, sizeof(path), "../man/%s.1", manpage);
        fp = fopen(path, "r");
    }
    
    if (fp == NULL) {
        printf("Cannot open manual page '%s.1'\n", manpage);
        return 1;
    }
    
    // Display man page (simple text display, skip troff commands)
    char line[1024];
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Remove trailing newline
        line[strcspn(line, "\n")] = '\0';
        
        // Skip troff commands
        if (line[0] == '.') {
            if (strncmp(line, ".SH", 3) == 0) {
                printf("\n\033[1m%s\033[0m\n", line + 4);
                printf("========================================\n");
            } else if (strncmp(line, ".TP", 3) == 0) {
                printf("  ");
            } else if (strncmp(line, ".B", 2) == 0) {
                printf("\033[1m%s\033[0m ", line + 3);
            } else if (strncmp(line, ".I", 2) == 0) {
                printf("\033[3m%s\033[0m ", line + 3);
            } else if (strncmp(line, ".TH", 3) == 0) {
                // Skip title header
            } else if (strncmp(line, ".fi", 3) == 0) {
                printf("\n");
            } else if (strncmp(line, ".nf", 3) == 0) {
                printf("\n");
            }
        } else {
            // Regular text
            printf("%s\n", line);
        }
    }
    
    fclose(fp);
    printf("\n");
    return 0;
}

// Main builtin dispatcher
int execute_builtin(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        return builtin_exit(args);
    } else if (strcmp(args[0], "cd") == 0) {
        return builtin_cd(args);
    } else if (strcmp(args[0], "env") == 0) {
        return builtin_env(args);
    } else if (strcmp(args[0], "setenv") == 0) {
        return builtin_setenv(args);
    } else if (strcmp(args[0], "unsetenv") == 0) {
        return builtin_unsetenv(args);
    } else if (strcmp(args[0], "alias") == 0) {
        return builtin_alias(args);
    } else if (strcmp(args[0], "path") == 0) {
        return builtin_path(args);
    } else if (strcmp(args[0], "man") == 0) {
        return builtin_man(args);
    }
    return -1;  // Not a builtin
}
