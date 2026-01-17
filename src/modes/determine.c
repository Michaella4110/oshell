#include "../include/shell.h"
#include "../include/errors.h"
#include <unistd.h>

shell_mode determine_mode(int argc, char **argv) {
    (void)argv;

    if (argc > 2) {
        print_error();
        return MODE_INVALID;
    } else if (argc == 2) {
        return MODE_BATCH;
    } else if (!isatty(STDIN_FILENO)) {
        return MODE_PIPE;
    } else {
        return MODE_INTERACTIVE;
    }
}
