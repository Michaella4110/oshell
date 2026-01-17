#include "../include/shell.h"
#include "../include/errors.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

extern shell_state g_state;

void free_commands(command *cmds) {
    if (!cmds) return;
    
    for (int i = 0; cmds[i].args != NULL || cmds[i].redir_file != NULL; i++) {
        if (cmds[i].args) {
            for (int j = 0; cmds[i].args[j] != NULL; j++) {
                free(cmds[i].args[j]);
            }
            free(cmds[i].args);
        }
        if (cmds[i].redir_file) {
            free(cmds[i].redir_file);
        }
    }
    free(cmds);
}

static int do_redirection(command *cmd) {
    if (cmd->redir_type == REDIR_NONE || cmd->redir_file == NULL) {
        return 0;
    }
    
    int fd = open(cmd->redir_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        print_error();
        return -1;
    }
    
    if (dup2(fd, STDOUT_FILENO) < 0) {
        close(fd);
        return -1;
    }
    if (dup2(fd, STDERR_FILENO) < 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static char *find_in_path(char *cmd) {
    if (strchr(cmd, '/') != NULL) {
        if (access(cmd, X_OK) == 0) return strdup(cmd);
        return NULL;
    }

    for (int i = 0; i < g_state.path_count; i++) {
        char *path = g_state.path_list[i];
        size_t len = strlen(path) + strlen(cmd) + 2;
        char *full = malloc(len);
        snprintf(full, len, "%s/%s", path, cmd);
        if (access(full, X_OK) == 0) return full;
        free(full);
    }
    return NULL;
}

// NEW: Function to execute alias by parsing it as a new command line
static int execute_alias(const char *alias_value, char **original_args) {
    // Create a command line from alias value + remaining arguments
    char *line = malloc(strlen(alias_value) + 1024); // Extra space for args
    if (!line) {
        print_error();
        return 1;
    }
    
    // Start with alias value
    strcpy(line, alias_value);
    
    // Append original arguments (skip the alias name itself)
    int has_space = (strlen(alias_value) > 0 && alias_value[strlen(alias_value)-1] != ' ');
    for (int i = 1; original_args[i] != NULL; i++) {
        if (has_space) {
            strcat(line, " ");
            has_space = 0;
        }
        strcat(line, original_args[i]);
        has_space = 1;
    }
    
    // Parse and execute the line
    command *cmds = parse_line(line);
    free(line);
    
    if (!cmds) {
        print_error();
        return 1;
    }
    
    int result = execute_sequence(cmds);
    free_commands(cmds);
    return result;
}

static int execute_single_command(command *cmd) {
    if (cmd == NULL || cmd->args == NULL || cmd->args[0] == NULL) {
        return 0;
    }

    // Check for alias BEFORE builtin
    char *alias_value = expand_alias(cmd->args[0]);
    if (alias_value) {
        // Execute alias by parsing it as a new command line
        int result = execute_alias(alias_value, cmd->args);
        g_state.exit_status = result;
        return result;
    }

    int builtin_result = execute_builtin(cmd->args);
    if (builtin_result != -1) {
        g_state.exit_status = builtin_result;
        return builtin_result;
    }

    sigset_t block_mask, old_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    sigprocmask(SIG_BLOCK, &block_mask, &old_mask);
    
    pid_t pid = fork();
    if (pid == -1) {
        print_error();
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
        g_state.exit_status = 1;
        return 1;
    }

    if (pid == 0) {
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
        
        struct sigaction sa;
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, NULL);
        
        if (do_redirection(cmd) < 0) _exit(1);
        
        char *cmd_path = find_in_path(cmd->args[0]);
        if (cmd_path == NULL) {
            print_error();
            _exit(127);
        }
        
        execv(cmd_path, cmd->args);
        print_error();
        _exit(126);
    } else {
        int status;
        waitpid(pid, &status, 0);
        
        sigset_t pending;
        sigpending(&pending);
        if (sigismember(&pending, SIGINT)) {
            siginfo_t info;
            struct timespec timeout = {0, 0};
            sigtimedwait(&block_mask, &info, &timeout);
        }
        
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
        
        if (WIFEXITED(status)) {
            g_state.exit_status = WEXITSTATUS(status);
            return g_state.exit_status;
        }
        
        g_state.exit_status = 1;
        return 1;
    }
}

int execute_sequence(command *cmds) {
    if (cmds == NULL) return 0;
    
    int last_status = 0;
    pid_t bg_pids[64];
    int bg_count = 0;
    
    for (int i = 0; cmds[i].args != NULL || cmds[i].redir_file != NULL; i++) {
        if (cmds[i].args == NULL && cmds[i].redir_file == NULL) continue;
        
        if (i > 0) {
            op_type prev_op = cmds[i-1].next_op;
            if (prev_op == OP_AND && last_status != 0) continue;
            if (prev_op == OP_OR && last_status == 0) continue;
        }
        
        if (cmds[i].next_op == OP_BG) {
            sigset_t block_mask, old_mask;
            sigemptyset(&block_mask);
            sigaddset(&block_mask, SIGINT);
            sigprocmask(SIG_BLOCK, &block_mask, &old_mask);
            
            pid_t pid = fork();
            
            if (pid == 0) {
                sigprocmask(SIG_SETMASK, &old_mask, NULL);
                
                struct sigaction sa;
                sa.sa_handler = SIG_DFL;
                sigemptyset(&sa.sa_mask);
                sa.sa_flags = 0;
                sigaction(SIGINT, &sa, NULL);
                
                if (do_redirection(&cmds[i]) < 0) _exit(1);
                
                // Check for alias in background job too
                char *alias_value = expand_alias(cmds[i].args[0]);
                if (alias_value) {
                    // Execute alias
                    char *line = malloc(strlen(alias_value) + 1024);
                    if (line) {
                        strcpy(line, alias_value);
                        for (int j = 1; cmds[i].args[j]; j++) {
                            strcat(line, " ");
                            strcat(line, cmds[i].args[j]);
                        }
                        command *alias_cmds = parse_line(line);
                        free(line);
                        if (alias_cmds) {
                            for (int k = 0; alias_cmds[k].args || alias_cmds[k].redir_file; k++) {
                                if (do_redirection(&alias_cmds[k]) < 0) _exit(1);
                                int builtin_result = execute_builtin(alias_cmds[k].args);
                                if (builtin_result != -1) {
                                    _exit(builtin_result);
                                }
                                char *cmd_path = find_in_path(alias_cmds[k].args[0]);
                                if (!cmd_path) {
                                    print_error();
                                    _exit(127);
                                }
                                execv(cmd_path, alias_cmds[k].args);
                                print_error();
                                _exit(126);
                            }
                            free_commands(alias_cmds);
                        }
                    }
                    _exit(1);
                }
                
                int builtin_result = execute_builtin(cmds[i].args);
                if (builtin_result != -1) {
                    _exit(builtin_result);
                }
                
                char *cmd_path = find_in_path(cmds[i].args[0]);
                if (cmd_path == NULL) {
                    print_error();
                    _exit(127);
                }
                execv(cmd_path, cmds[i].args);
                print_error();
                _exit(126);
            } else if (pid > 0) {
                sigprocmask(SIG_SETMASK, &old_mask, NULL);
                bg_pids[bg_count++] = pid;
                last_status = 0;
                continue;
            } else {
                sigprocmask(SIG_SETMASK, &old_mask, NULL);
                print_error();
                last_status = 1;
                continue;
            }
        } else {
            last_status = execute_single_command(&cmds[i]);
        }
        
        if (cmds[i].next_op == OP_NONE) break;
    }
    
    if (bg_count > 0) {
        sigset_t block_mask, old_mask;
        sigemptyset(&block_mask);
        sigaddset(&block_mask, SIGINT);
        sigprocmask(SIG_BLOCK, &block_mask, &old_mask);
        
        for (int j = 0; j < bg_count; j++) {
            int status;
            waitpid(bg_pids[j], &status, 0);
            
            sigset_t pending;
            sigpending(&pending);
            if (sigismember(&pending, SIGINT)) {
                siginfo_t info;
                struct timespec timeout = {0, 0};
                sigtimedwait(&block_mask, &info, &timeout);
            }
        }
        
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
    }
    
    g_state.exit_status = last_status;
    return last_status;
}
