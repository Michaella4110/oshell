#include "modes/modes.h"
#include "../include/shell.h"

int main(int argc, char **argv) {
    init_shell_state();

    shell_mode mode = determine_mode(argc, argv);

    switch (mode) {
        case MODE_INTERACTIVE:
            interactive_mode();
            break;
        case MODE_PIPE:
            pipe_mode();
            break;
        case MODE_BATCH:
            batch_mode(argv[1]);
            break;
        case MODE_INVALID:
        default:
            free_shell_state();
            return 1;
    }

    free_shell_state();
    return 0;
}
