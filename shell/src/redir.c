#include<stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/wait.h>
#include "redir.h"

Command parse_tokens_for_execution(token* list, int count) {
    Command cmd = {0};
    cmd.argv = malloc(count * sizeof(char*));
    int argc = 0;

    for (int i = 0; i < count - 1; i++) {
        if (list[i].type == input) {
            i++;
            if (i < count - 1) cmd.input_file = list[i].name;
        } else if (list[i].type == output) { // ✅ Handle '>'
            i++;
            if (i < count - 1) {
                cmd.output_file = list[i].name;
                cmd.append_output = false; // Overwrite
            }
        } else if (list[i].type == append) { // ✅ Handle '>>'
            i++;
            if (i < count - 1) {
                cmd.output_file = list[i].name;
                cmd.append_output = true; // Append
            }
        } else {
            cmd.argv[argc++] = list[i].name;
        }
    }
    cmd.argv[argc] = NULL;
    return cmd;
}

void execute_redirected_command(Command cmd) {
    int input_fd = -1;
    int output_fd = -1;

    // --- Setup Phase (before fork) ---

    // Handle input redirection
    if (cmd.input_file) {
        input_fd = open(cmd.input_file, O_RDONLY);
        if (input_fd == -1) { perror(cmd.input_file); return; }
    }

    // ✅ Handle output redirection
    if (cmd.output_file) {
        int open_flags;
        if (cmd.append_output) {
            // Flags for '>>': Create if not exists, Write-only, Append
            open_flags = O_CREAT | O_WRONLY | O_APPEND;
        } else {
            // Flags for '>': Create if not exists, Write-only, Truncate (wipe)
            open_flags = O_CREAT | O_WRONLY | O_TRUNC;
        }
        // Create the file with standard read/write permissions for user/group
        output_fd = open(cmd.output_file, open_flags, 0644);
        if (output_fd == -1) { perror(cmd.output_file); if (input_fd != -1) close(input_fd); return; }
    }

    // --- Execution Phase ---
    pid_t pid = fork();
    if (pid == 0) {
        // --- Child Process ---
        if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO); // Redirect standard output (1)
            close(output_fd);
        }
        execvp(cmd.argv[0], cmd.argv);
        perror(cmd.argv[0]);
        exit(EXIT_FAILURE);
    } else {
        // --- Parent Process ---
        if (input_fd != -1) close(input_fd);
        if (output_fd != -1) close(output_fd);
        waitpid(pid, NULL, 0);
    }
}