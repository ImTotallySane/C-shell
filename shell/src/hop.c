#define _POSIX_C_SOURCE 200809L // For PATH_MAX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include<token.h>
#include "hop.h"

// Static variables for state management
static char previous_cwd[PATH_MAX];
static bool has_previous_cwd = false;

// --- Public Function ---

int execute_hop(token* tokens, int token_count, const char* shell_home_dir) {
    // Case 1: "hop" with no arguments (equivalent to "hop ~")
    if (token_count <= 2) {
        char current_dir[PATH_MAX];
        if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
            perror("hop: getcwd");
            return 0; // Directory not changed
        }

        if (shell_home_dir == NULL) {
            fprintf(stderr, "hop: home directory not set\n");
            return 0; // Directory not changed
        }

        if (chdir(shell_home_dir) == 0) {
            strcpy(previous_cwd, current_dir);
            has_previous_cwd = true;
            return 1; // Directory successfully changed
        } else {
            perror("hop: chdir");
            return 0; // Directory not changed
        }
    }

    // Case 2: "hop" with one or more arguments
    int directory_changed = 0; // Flag to track if any change occurred
    for (int i = 1; i < token_count - 1; i++) {
        const char* arg = tokens[i].name;
        if (arg == NULL) continue;

        if (strcmp(arg, ".") == 0) {
            continue; // No change for "."
        }

        char current_dir[PATH_MAX];
        if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
            perror("hop: getcwd");
            break;
        }

        int chdir_result = -1;

        if (strcmp(arg, "~") == 0) {
            chdir_result = (shell_home_dir != NULL) ? chdir(shell_home_dir) : -1;
        } else if (strcmp(arg, "..") == 0) {
            chdir_result = chdir("..");
        } else if (strcmp(arg, "-") == 0) {
            if (!has_previous_cwd) {
                continue; // No change if no previous directory
            }
            chdir_result = chdir(previous_cwd);
            if (chdir_result == 0) {
                strcpy(previous_cwd, current_dir);
                directory_changed = 1; // Mark that a change occurred
                continue;
            }
        } else {
            chdir_result = chdir(arg);
        }

        if (chdir_result == 0) {
            strcpy(previous_cwd, current_dir);
            has_previous_cwd = true;
            directory_changed = 1; // Mark that a change occurred
        } else {
            fprintf(stderr, "No such directory!\n");
            break; // Stop on failure
        }
    }
    
    return directory_changed;
}