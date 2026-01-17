#include "../include/shell.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

extern shell_state g_state;

static char *get_exit_status_str(void) {
    char *str = malloc(12);
    if (str) {
        snprintf(str, 12, "%d", g_state.exit_status);
    }
    return str;
}

static char *get_shell_pid_str(void) {
    char *str = malloc(12);
    if (str) {
        snprintf(str, 12, "%d", (int)g_state.shell_pid);
    }
    return str;
}

static char *get_env_var(const char *name) {
    // SPECIAL HANDLING FOR PATH - return OShell's internal path, not system PATH
    if (strcmp(name, "PATH") == 0) {
        // Build OShell's internal path as colon-separated string
        if (g_state.path_count == 0) {
            return strdup("");
        }
        
        // Calculate total length needed
        size_t total_len = 0;
        for (int i = 0; i < g_state.path_count; i++) {
            total_len += strlen(g_state.path_list[i]) + 1; // +1 for colon or null terminator
        }
        
        char *path_str = malloc(total_len);
        if (!path_str) return strdup("");
        
        path_str[0] = '\0';
        for (int i = 0; i < g_state.path_count; i++) {
            if (i > 0) {
                strcat(path_str, ":");
            }
            strcat(path_str, g_state.path_list[i]);
        }
        
        return path_str;
    }
    
    // For all other variables, use system environment
    char *value = getenv(name);
    if (value) {
        return strdup(value);
    }
    return strdup("");
}

char *expand_variables(char *str) {
    if (!str || strchr(str, '$') == NULL) {
        return strdup(str ? str : "");
    }
    
    size_t buf_size = strlen(str) * 2 + 1;
    char *result = malloc(buf_size);
    if (!result) return NULL;
    
    size_t result_pos = 0;
    
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '$' && str[i+1] != '\0') {
            i++;
            
            if (str[i] == '?') {
                char *exit_str = get_exit_status_str();
                if (exit_str) {
                    size_t exit_len = strlen(exit_str);
                    if (result_pos + exit_len + 1 >= buf_size) {
                        buf_size = (result_pos + exit_len + 1) * 2;
                        char *new_result = realloc(result, buf_size);
                        if (!new_result) {
                            free(exit_str);
                            free(result);
                            return NULL;
                        }
                        result = new_result;
                    }
                    strcpy(&result[result_pos], exit_str);
                    result_pos += exit_len;
                    free(exit_str);
                }
                continue;
            } else if (str[i] == '$') {
                char *pid_str = get_shell_pid_str();
                if (pid_str) {
                    size_t pid_len = strlen(pid_str);
                    if (result_pos + pid_len + 1 >= buf_size) {
                        buf_size = (result_pos + pid_len + 1) * 2;
                        char *new_result = realloc(result, buf_size);
                        if (!new_result) {
                            free(pid_str);
                            free(result);
                            return NULL;
                        }
                        result = new_result;
                    }
                    strcpy(&result[result_pos], pid_str);
                    result_pos += pid_len;
                    free(pid_str);
                }
                continue;
            }
            
            size_t start = i;
            while (str[i] != '\0' && 
                   (isalnum((unsigned char)str[i]) || str[i] == '_')) {
                i++;
            }
            size_t var_len = i - start;
            i--;
            
            if (var_len > 0) {
                char var_name[256];
                if (var_len < sizeof(var_name)) {
                    strncpy(var_name, &str[start], var_len);
                    var_name[var_len] = '\0';
                    
                    char *value = get_env_var(var_name);
                    if (value) {
                        size_t value_len = strlen(value);
                        if (result_pos + value_len + 1 >= buf_size) {
                            buf_size = (result_pos + value_len + 1) * 2;
                            char *new_result = realloc(result, buf_size);
                            if (!new_result) {
                                free(value);
                                free(result);
                                return NULL;
                            }
                            result = new_result;
                        }
                        strcpy(&result[result_pos], value);
                        result_pos += value_len;
                        free(value);
                    }
                }
            }
        } else {
            if (result_pos + 2 >= buf_size) {
                buf_size *= 2;
                char *new_result = realloc(result, buf_size);
                if (!new_result) {
                    free(result);
                    return NULL;
                }
                result = new_result;
            }
            result[result_pos++] = str[i];
        }
    }
    
    result[result_pos] = '\0';
    
    char *final = strdup(result);
    free(result);
    return final;
}
