#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "pipe.h"
#include "reveal.h"
#include "hop.h"

Pipeline parse_tokens_for_pipeline(token* list, int count) {
    Pipeline pipeline = {0};
    int cmd_start_index = 0;

    for (int i = 0; i < count - 1; i++) {
        if (list[i].type == token_pipe) {
            // Found a separator, process the command before it
            Command* cmd = &pipeline.commands[pipeline.num_commands];
            *cmd = parse_tokens_for_execution(&list[cmd_start_index], i - cmd_start_index + 1);
            pipeline.num_commands++;
            cmd_start_index = i + 1;
        }
    }
    // Process the final command in the pipeline (after the last pipe or the only command)
    Command* cmd = &pipeline.commands[pipeline.num_commands];
    *cmd = parse_tokens_for_execution(&list[cmd_start_index], count - 1 - cmd_start_index + 1);
    pipeline.num_commands++;
    
    return pipeline;
}

void execute_pipeline(Pipeline pipeline, char* homedir, char* prev_dir, bool has_previous_cwd) {
    if (pipeline.num_commands == 0) return;

    int input_fd = STDIN_FILENO;
    pid_t pids[pipeline.num_commands];

    for (int i = 0; i < pipeline.num_commands; i++) {
        Command* cmd = &pipeline.commands[i];
        int pipe_fds[2];

        if (i < pipeline.num_commands - 1) {
            if (pipe(pipe_fds) == -1) { perror("pipe"); return; }
        }

        pids[i] = fork();
        if (pids[i] == 0) {
            // --- Child Process ---
            if (input_fd != STDIN_FILENO) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            if (cmd->input_file) {
                int fd = open(cmd->input_file, O_RDONLY);
                if (fd == -1) { perror(cmd->input_file); exit(EXIT_FAILURE); }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            if (i < pipeline.num_commands - 1) {
                close(pipe_fds[0]);
                dup2(pipe_fds[1], STDOUT_FILENO);
                close(pipe_fds[1]);
            }
            if (cmd->output_file) {
                int flags = O_CREAT | O_WRONLY | (cmd->append_output ? O_APPEND : O_TRUNC);
                int fd = open(cmd->output_file, flags, 0644);
                if (fd == -1) { perror(cmd->output_file); exit(EXIT_FAILURE); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            // --- Dispatcher for built-ins vs. external ---
            if (strcmp(cmd->argv[0], "reveal") == 0) {
                token dummy_tokens[2] = { {.name=cmd->argv[0]}, {.type=eof} };
                execute_reveal(dummy_tokens, 2, homedir, prev_dir, has_previous_cwd);
                exit(EXIT_SUCCESS);
            } else {
                execvp(cmd->argv[0], cmd->argv);
                perror(cmd->argv[0]);
                exit(EXIT_FAILURE);
            }
        }

        // --- Parent Process ---
        if (input_fd != STDIN_FILENO) close(input_fd);
        if (i < pipeline.num_commands - 1) {
            close(pipe_fds[1]);
            input_fd = pipe_fds[0];
        }
    }

    for (int i = 0; i < pipeline.num_commands; i++) {
        waitpid(pids[i], NULL, 0);
    }
    for (int i = 0; i < pipeline.num_commands; i++) {
        free(pipeline.commands[i].argv);
    }
}