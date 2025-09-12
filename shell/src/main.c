#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <limits.h>
#include "prompt.h"
#include "parser.h"
#include "execute.h"
#include "log.h"
#include "jobs.h"
#include "signals.h"

char SHELL_HOME_DIR[PATH_MAX];
char PREV_DIR[PATH_MAX];
pid_t SHELL_PID;
pid_t FOREGROUND_CHILD_PID = -1;

int main() {

    SHELL_PID = getpid();
    if (getcwd(SHELL_HOME_DIR, sizeof(SHELL_HOME_DIR)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    strcpy(PREV_DIR, "");
    init_log();
    setup_signal_handlers();
    
    char *input = NULL;
    size_t len = 0;
    
    while (1) {
        check_background_processes();
        display_prompt();

        ssize_t nread = getline(&input, &len, stdin);

        if (nread == -1) { // EOF (Ctrl+D)
            printf("logout\n");
            kill_all_jobs();
            break; // Exit the loop to clean up and exit
        }

        // Ignore empty lines
        if (strcmp(input, "\n") == 0) {
            continue;
        }
        
        // Strip trailing newline for logging
        input[strcspn(input, "\n")] = 0;

        if (should_log_command(input)) {
            add_to_log(input);
        }

        ShellCommand *shell_cmd = parse_input(input);
        if (shell_cmd == NULL) {
            fprintf(stderr, "Invalid Syntax!\n");
        } else {
            execute_shell_command(shell_cmd);
            free_shell_command(shell_cmd);
        }

        // DO NOT free(input) here. getline will reuse the buffer.
    }

    // Free the buffer once when the shell exits.
    free(input);
    // --- FIX ENDS HERE ---

    return 0;
}