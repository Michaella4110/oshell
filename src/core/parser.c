#include "../include/shell.h"
#include "../include/errors.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#define MAX_COMMANDS 64
#define MAX_ARGS_PER_CMD 64

extern shell_state g_state;

static int is_operator_char(char c) {
    return (c == '>' || c == '<' || c == '|' || c == '&' || c == ';');
}

static int is_double_op(char a, char b) {
    return ((a == '&' && b == '&') || (a == '|' && b == '|'));
}

static op_type token_to_op(const char *tok) {
    if (strcmp(tok, ";") == 0) return OP_SEQ;
    if (strcmp(tok, "&&") == 0) return OP_AND;
    if (strcmp(tok, "||") == 0) return OP_OR;
    if (strcmp(tok, "&") == 0) return OP_BG;
    return OP_NONE;
}

static void strip_quotes(char *str) {
    if (!str) return;
    size_t len = strlen(str);
    if (len >= 2) {
        char first = str[0];
        char last = str[len-1];
        if ((first == '\'' && last == '\'') || (first == '"' && last == '"')) {
            memmove(str, str + 1, len - 2);
            str[len - 2] = '\0';
        }
    }
}

static char *process_token(char *token) {
    if (!token) return strdup("");
    
    char *token_copy = strdup(token);
    strip_quotes(token_copy);
    
    if (strchr(token_copy, '$') != NULL) {
        char *expanded = expand_variables(token_copy);
        free(token_copy);
        return expanded ? expanded : strdup("");
    }
    
    return token_copy;
}

command *parse_line(char *line) {
    if (!line || *line == '\0') return NULL;
    
    char *comment = strchr(line, '#');
    if (comment) *comment = '\0';
    
    char *end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end)) {
        *end-- = '\0';
    }
    
    char *start = line;
    while (*start && isspace((unsigned char)*start)) start++;
    if (*start == '\0') return NULL;
    
    command *cmds = calloc(MAX_COMMANDS, sizeof(command));
    if (!cmds) return NULL;
    
    int cmd_idx = 0;
    char *args[MAX_ARGS_PER_CMD];
    int arg_idx = 0;
    
    cmds[cmd_idx].args = NULL;
    cmds[cmd_idx].redir_type = REDIR_NONE;
    cmds[cmd_idx].redir_file = NULL;
    cmds[cmd_idx].next_op = OP_NONE;
    
    int i = 0;
    int len = strlen(start);
    
    while (i < len && cmd_idx < MAX_COMMANDS - 1) {
        while (i < len && isspace((unsigned char)start[i])) i++;
        if (i >= len) break;
        
        if (start[i] == '>') {
            if (cmds[cmd_idx].redir_type != REDIR_NONE) {
                print_error();
                free_commands(cmds);
                return NULL;
            }
            i++;
            
            while (i < len && isspace((unsigned char)start[i])) i++;
            
            if (i >= len) {
                print_error();
                free_commands(cmds);
                return NULL;
            }
            
            int file_start = i;
            while (i < len && !isspace((unsigned char)start[i]) && 
                   !is_operator_char(start[i])) {
                i++;
            }
            
            if (i == file_start) {
                print_error();
                free_commands(cmds);
                return NULL;
            }
            
            char *filename = malloc(i - file_start + 1);
            strncpy(filename, &start[file_start], i - file_start);
            filename[i - file_start] = '\0';
            
            cmds[cmd_idx].redir_type = REDIR_OUT;
            cmds[cmd_idx].redir_file = filename;
            continue;
        }
        
        if (is_operator_char(start[i])) {
            if (arg_idx == 0 && cmds[cmd_idx].redir_type == REDIR_NONE && cmd_idx == 0) {
                print_error();
                free_commands(cmds);
                return NULL;
            }
            
            if (arg_idx > 0 || cmds[cmd_idx].redir_type != REDIR_NONE) {
                args[arg_idx] = NULL;
                cmds[cmd_idx].args = malloc((arg_idx + 1) * sizeof(char *));
                for (int k = 0; k < arg_idx; k++) {
                    cmds[cmd_idx].args[k] = process_token(args[k]);
                    free(args[k]);
                }
                cmds[cmd_idx].args[arg_idx] = NULL;
                arg_idx = 0;
            } else {
                print_error();
                free_commands(cmds);
                return NULL;
            }
            
            char op[3] = {0};
            if (i+1 < len && is_double_op(start[i], start[i+1])) {
                op[0] = start[i];
                op[1] = start[i+1];
                op[2] = '\0';
                i += 2;
            } else {
                op[0] = start[i];
                op[1] = '\0';
                i++;
            }
            
            cmds[cmd_idx].next_op = token_to_op(op);
            cmd_idx++;
            
            cmds[cmd_idx].args = NULL;
            cmds[cmd_idx].redir_type = REDIR_NONE;
            cmds[cmd_idx].redir_file = NULL;
            cmds[cmd_idx].next_op = OP_NONE;
            continue;
        }
        
        int arg_start = i;
        char quote = 0;
        while (i < len) {
            if (quote == 0) {
                if (isspace((unsigned char)start[i])) break;
                if (is_operator_char(start[i])) break;
                if (start[i] == '\'' || start[i] == '"') {
                    quote = start[i];
                }
            } else {
                if (start[i] == quote) {
                    quote = 0;
                }
            }
            i++;
        }
        
        if (quote != 0) {
            print_error();
            free_commands(cmds);
            return NULL;
        }
        
        if (i > arg_start) {
            int tok_len = i - arg_start;
            char *arg = malloc(tok_len + 1);
            strncpy(arg, &start[arg_start], tok_len);
            arg[tok_len] = '\0';
            
            if (arg_idx < MAX_ARGS_PER_CMD - 1) {
                args[arg_idx++] = arg;
            } else {
                free(arg);
            }
        }
    }
    
    if (arg_idx > 0 || cmds[cmd_idx].redir_type != REDIR_NONE) {
        args[arg_idx] = NULL;
        cmds[cmd_idx].args = malloc((arg_idx + 1) * sizeof(char *));
        for (int k = 0; k < arg_idx; k++) {
            if (args[k]) {
                cmds[cmd_idx].args[k] = process_token(args[k]);
                free(args[k]);
            }
        }
        cmds[cmd_idx].args[arg_idx] = NULL;
        cmd_idx++;
    } else if (cmd_idx > 0) {
        print_error();
        free_commands(cmds);
        return NULL;
    }
    
    cmds[cmd_idx].args = NULL;
    cmds[cmd_idx].redir_type = REDIR_NONE;
    cmds[cmd_idx].redir_file = NULL;
    cmds[cmd_idx].next_op = OP_NONE;
    
    return cmds;
}
