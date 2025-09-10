#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "exec.h"

/**
 * @brief (Internal Helper) Converts a token list to a NULL-terminated
 * char* array (argv format) required by execvp.
 * @param list The token array from the tokenizer.
 * @param count The number of tokens.
 * @return A dynamically allocated, NULL-terminated char** array. The caller
 * is responsible for freeing this memory.
 */
static char** create_argv(token* list, int count) {
    // Allocate space for 'count' pointers (count-1 arguments + 1 NULL terminator)
    char** argv = malloc(count * sizeof(char*));
    if (argv == NULL) {
        perror("malloc");
        return NULL;
    }

    // Copy the name pointer from each token into the new array
    for (int i = 0; i < count - 1; i++) {
        argv[i] = list[i].name;
    }

    // The last element of an argv array must be NULL for execvp
    argv[count - 1] = NULL;
    return argv;
}

void execute_external_command(token* list, int count) {
    // Prepare the argument array for execvp
    char** argv = create_argv(list, count);
    if (argv == NULL) {
        return; // Error handled in create_argv
    }

    pid_t pid = fork();

    if (pid < 0) {
        // Forking failed
        perror("fork");
        free(argv);
        return;
    }

    if (pid == 0) {
        // --- This is the Child Process ---
        execvp(argv[0], argv);

        // If execvp() returns, it means an error occurred.
        // For example, the command was not found in the system's PATH.
        //perror(argv[0]);
        free(argv); // Free memory before exiting
        exit(EXIT_FAILURE);
    } else {
        // --- This is the Parent Process (the shell) ---
        // Wait for the child process to complete its execution.
        waitpid(pid, NULL, 0);
    }

    // The parent process must free the memory for the argv array.
    free(argv);
}