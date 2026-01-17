#ifndef MODES_H
#define MODES_H

#include "../include/shell.h"

shell_mode determine_mode(int argc, char **argv);
void interactive_mode(void);
void pipe_mode(void);
void batch_mode(const char *filename);

#endif
