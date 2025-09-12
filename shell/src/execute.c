#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#include "execute.h"
#include "parser.h"
#include "intrinsics.h"
#include "jobs.h"

extern pid_t FOREGROUND_CHILD_PID;

static int preflight_check_redirects(CommandGroup *group) {
    for (AtomicCommand *ac = group->head_atomic; ac != NULL; ac = ac->next) {
        for (Redirect *r = ac->redirects; r != NULL; r = r->next) {
            int fd;
            if (r->type == '<') {
                fd = open(r->filename, O_RDONLY);
                if (fd == -1) {
                    fprintf(stderr, "No such file or directory\n");
                    return -1;
                }
            } else { // '>' or 'a'
                int flags = O_WRONLY | O_CREAT | (r->type == 'a' ? O_APPEND : O_TRUNC);
                fd = open(r->filename, flags, 0644);
                if (fd == -1) {
                    fprintf(stderr, "Unable to create file for writing\n");
                    return -1;
                }
            }
            close(fd);
        }
    }
    return 0;
}

static void apply_final_redirects(AtomicCommand *cmd) {
    char *final_input = NULL;
    char *final_output = NULL;
    int is_append = 0;

    for (Redirect *r = cmd->redirects; r != NULL; r = r->next) {
        if (r->type == '<') {
            final_input = r->filename;
        } else {
            final_output = r->filename;
            is_append = (r->type == 'a');
        }
    }

    if (final_input) {
        int fd_in = open(final_input, O_RDONLY);
        if (fd_in != -1) {
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
    }
    if (final_output) {
        int flags = O_WRONLY | O_CREAT | (is_append ? O_APPEND : O_TRUNC);
        int fd_out = open(final_output, flags, 0644);
         if (fd_out != -1) {
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
    }
}

// ############## LLM Generated Code Begins ##############

static pid_t execute_piped_group(CommandGroup *group) {
    AtomicCommand *atomic_cmd = group->head_atomic;
    if (!atomic_cmd) return -1;

    // Perform the pre-flight check on all redirections BEFORE forking.
    if (preflight_check_redirects(group) != 0) {
        return -1; // Abort execution if any redirection is invalid.
    }
    
    // Handle single intrinsic command
    if (!atomic_cmd->next && is_intrinsic(atomic_cmd->name)) {
        int saved_stdin = dup(STDIN_FILENO);
        int saved_stdout = dup(STDOUT_FILENO);
        
        apply_final_redirects(atomic_cmd);
        
        execute_intrinsic(atomic_cmd);
        
        dup2(saved_stdin, STDIN_FILENO);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdin);
        close(saved_stdout);
        
        return 0;
    }

    int num_pipes = 0;
    for (AtomicCommand *ac = atomic_cmd; ac->next != NULL; ac = ac->next) num_pipes++;

    int pipe_fds[num_pipes][2];
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipe_fds[i]) == -1) { perror("pipe"); return -1; }
    }

    AtomicCommand *current_cmd = atomic_cmd;
    int i = 0;
    pid_t last_pid = -1;

    while (current_cmd) {
        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return -1; }

        if (pid == 0) { // Child process
            apply_final_redirects(current_cmd);

            if (i > 0) {
                dup2(pipe_fds[i - 1][0], STDIN_FILENO);
            }
            if (i < num_pipes) {
                dup2(pipe_fds[i][1], STDOUT_FILENO);
            }
            
            for (int j = 0; j < num_pipes; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            
            if (is_intrinsic(current_cmd->name)) {
                execute_intrinsic(current_cmd);
                exit(EXIT_SUCCESS);
            } else {
                execvp(current_cmd->name, current_cmd->args);
                fprintf(stderr, "%s: Command not found!\n", current_cmd->name);
                exit(EXIT_FAILURE);
            }
        } else { // Parent process
            last_pid = pid;
        }
        current_cmd = current_cmd->next;
        i++;
    }

    for (int j = 0; j < num_pipes; j++) {
        close(pipe_fds[j][0]);
        close(pipe_fds[j][1]);
    }

    return last_pid;
}

// ############## LLM Generated Code Ends ##############

void execute_shell_command(ShellCommand *shell_cmd) {
    CommandGroup *current_group = shell_cmd->head_group;

    while (current_group) {
        int is_bg = (current_group->separator == '&') || (current_group->next == NULL && shell_cmd->is_background);

        pid_t last_pid = execute_piped_group(current_group);

// ############## LLM Generated Code Begins ##############

        if (last_pid == -1) {
            // Error message is now printed inside preflight_check_redirects
        } else if (is_bg) {
            if (last_pid > 0) {
                 add_job(last_pid, current_group->head_atomic->name, RUNNING);
                 printf("[%d] %d\n", last_pid, last_pid);
            }
        } else {
            // If it's a foreground job, WAIT for it to complete.
            if (last_pid > 0) {
                FOREGROUND_CHILD_PID = last_pid;
                int status;
                waitpid(last_pid, &status, WUNTRACED);

                if (WIFSTOPPED(status)) {
                    add_job(last_pid, current_group->head_atomic->name, STOPPED);
                    printf("\n[%d] Stopped %s\n", last_pid, current_group->head_atomic->name);
                }
                FOREGROUND_CHILD_PID = -1;
            }
        }

// ############## LLM Generated Code Ends ##############

        current_group = current_group->next;
    }
}