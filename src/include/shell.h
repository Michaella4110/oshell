#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>

typedef enum {
    MODE_INTERACTIVE,
    MODE_PIPE,
    MODE_BATCH,
    MODE_INVALID
} shell_mode;

typedef enum {
    REDIR_NONE,
    REDIR_OUT
} redir_type;

typedef enum {
    OP_NONE,
    OP_SEQ,
    OP_AND,
    OP_OR,
    OP_BG
} op_type;

typedef struct command {
    char **args;
    redir_type redir_type;
    char *redir_file;
    op_type next_op;
} command;

typedef struct {
    char **path_list;
    int path_count;
    char *pwd;
    char *oldpwd;
    int exit_status;
    pid_t shell_pid;
} shell_state;

extern shell_state g_state;

// Function prototypes
command *parse_line(char *line);
int execute_sequence(command *cmds);
int execute_builtin(char **args);
void init_shell_state(void);
void free_shell_state(void);
void free_commands(command *cmds);
char *expand_variables(char *str);
char *expand_alias(const char *name);

#endif
