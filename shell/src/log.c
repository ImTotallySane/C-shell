#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "log.h"
#include "parser.h"
#include "execute.h"

#define MAX_LOG_SIZE 15
#define LOG_FILE_NAME ".shell_log"

extern char SHELL_HOME_DIR[PATH_MAX];

static char *command_log[MAX_LOG_SIZE];
static int log_count = 0;

// Helper to get the full, absolute path to the log file
void get_log_file_path(char *path_buffer, size_t buffer_size) {
    snprintf(path_buffer, buffer_size, "%s/%s", SHELL_HOME_DIR, LOG_FILE_NAME);
}

void init_log() {
    char log_path[PATH_MAX];
    get_log_file_path(log_path, sizeof(log_path));

    FILE *file = fopen(log_path, "r");
    if (!file) return;

    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, file) != -1 && log_count < MAX_LOG_SIZE) {

    // ############## LLM Generated Code Begins ##############

        line[strcspn(line, "\n")] = 0;
        command_log[log_count++] = strdup(line);

    // ############## LLM Generated Code Ends ##############

    }
    free(line);
    fclose(file);
}

void save_log() {
    char log_path[PATH_MAX];
    get_log_file_path(log_path, sizeof(log_path));

    FILE *file = fopen(log_path, "w");
    if (!file) {
        perror("fopen log for writing");
        return;
    }
    for (int i = 0; i < log_count; i++) {
        fprintf(file, "%s\n", command_log[i]);
    }
    fclose(file);
}

int should_log_command(const char *command) {
    char temp_cmd[strlen(command) + 1];
    strcpy(temp_cmd, command);
    temp_cmd[strcspn(temp_cmd, "\n")] = 0;

    char *first_word = strtok(temp_cmd, " \t\n");
    if (first_word && strcmp(first_word, "log") == 0) {
        return 0;
    }

    if (log_count > 0 && strcmp(command_log[log_count-1], command) == 0) {
        return 0;
    }

    return 1;
}

void add_to_log(const char *command) {
    char clean_command[strlen(command) + 1];
    strcpy(clean_command, command);
    clean_command[strcspn(clean_command, "\n")] = 0;

    if (log_count == MAX_LOG_SIZE) {
        free(command_log[0]);
        for (int i = 0; i < MAX_LOG_SIZE - 1; i++) {
            command_log[i] = command_log[i + 1];
        }
        command_log[MAX_LOG_SIZE - 1] = strdup(clean_command);
    } else {
        command_log[log_count++] = strdup(clean_command);
    }
    save_log();
}

void handle_log_command(char **args) {
    if (args[1] == NULL) { // log
        for (int i = 0; i < log_count; i++) {
            printf("%s\n", command_log[i]);
        }
    } else if (strcmp(args[1], "purge") == 0) { // log purge
        for (int i = 0; i < log_count; i++) {
            free(command_log[i]);
        }
        log_count = 0;
        
        char log_path[PATH_MAX];
        get_log_file_path(log_path, sizeof(log_path));
        remove(log_path); 
        
    } else if (strcmp(args[1], "execute") == 0) { // log execute <index>
        if (args[2] == NULL) {
            fprintf(stderr, "log: execute requires an index\n");
            return;
        }
        int index = atoi(args[2]);
        if (index <= 0 || index > log_count) {
            fprintf(stderr, "log: invalid index\n");
            return;
        }
        // This indexing is tricky. The prompt says "newest to oldest"
        // and 1-indexed. Our array is oldest-to-newest and 0-indexed.
        // So `log execute 1` should be the last command in our array.
        char *cmd_to_exec = command_log[log_count - index];
        
        ShellCommand *shell_cmd = parse_input(cmd_to_exec);
        if (shell_cmd) {
            execute_shell_command(shell_cmd);
            free_shell_command(shell_cmd);
        } else {
            fprintf(stderr, "Invalid Syntax in log!\n");
        }
    }
}
