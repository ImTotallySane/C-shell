#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>

#include "intrinsics.h"
#include "log.h"
#include "jobs.h"

extern char SHELL_HOME_DIR[];
extern char PREV_DIR[];

static void handle_hop(char **args) {
    char target_dir[PATH_MAX];
    int arg_idx = 1;

    if (args[arg_idx] == NULL) {
        strcpy(target_dir, SHELL_HOME_DIR);
        char current_dir[PATH_MAX];
        if (getcwd(current_dir, sizeof(current_dir)) == NULL)
             { 
                perror("getcwd"); 
                return; 
            }
        if (chdir(target_dir) == 0) {
            strcpy(PREV_DIR, current_dir);
        } else {
            perror("hop");
        }
        return;
    }

    while (args[arg_idx] != NULL) {
        char current_dir[PATH_MAX];
        if (getcwd(current_dir, sizeof(current_dir)) == NULL) 
            { 
                perror("getcwd");
                arg_idx++;
                continue;
            }
        if (strcmp(args[arg_idx], "~") == 0) {
            strcpy(target_dir, SHELL_HOME_DIR);
        } else if (strcmp(args[arg_idx], ".") == 0) {
            arg_idx++;
            continue;
        } else if (strcmp(args[arg_idx], "..") == 0) {
            strcpy(target_dir, "..");
        } else if (strcmp(args[arg_idx], "-") == 0) {
            if (strlen(PREV_DIR) == 0) {
                fprintf(stderr, "hop: OLDPWD not set\n");
                arg_idx++;
                continue;
            }
            strcpy(target_dir, PREV_DIR);
        } else {
            strcpy(target_dir, args[arg_idx]);
        }

        if (chdir(target_dir) == 0) {
            strcpy(PREV_DIR, current_dir);
        } else {
            fprintf(stderr, "hop: No such directory!\n");
        }
        arg_idx++;
    }
}

static void handle_reveal(char **args) {
    int show_all = 0;
    int long_format = 0;
    char *dir_arg = NULL;
    int non_flag_args = 0;

    for (int i = 1; args[i] != NULL; i++) {

        // ############## LLM Generated Code Begins ##############

        // Correctly distinguish between the special '-' argument and flags like '-a' or '-al'.
        if (strcmp(args[i], "-") == 0) {
            dir_arg = args[i];
            non_flag_args++;
        } else if (args[i][0] == '-') {
            for (size_t j = 1; j < strlen(args[i]); j++) {
                if (args[i][j] == 'a') show_all = 1;
                else if (args[i][j] == 'l') long_format = 1;
            }
        } else {
            dir_arg = args[i];
            non_flag_args++;
        }
    }
    
    if (non_flag_args > 1) {
        fprintf(stderr, "reveal: Invalid Syntax!\n");
        return;
    }

        // ############## LLM Generated Code Ends ##############

    char target_path[PATH_MAX];
    if (dir_arg == NULL) {
        if (getcwd(target_path, sizeof(target_path)) == NULL) 
            { 
                perror("getcwd"); 
                return;
            }
    } else if (strcmp(dir_arg, "~") == 0) {
        strcpy(target_path, SHELL_HOME_DIR);
    } else if (strcmp(dir_arg, ".") == 0) {
        if (getcwd(target_path, sizeof(target_path)) == NULL) { perror("getcwd"); return; }
    } else if (strcmp(dir_arg, "..") == 0) {
         if (getcwd(target_path, sizeof(target_path)) == NULL) { perror("getcwd"); return; }
         char *last_slash = strrchr(target_path, '/');
         if (last_slash != target_path) *last_slash = '\0';
         else *(last_slash + 1) = '\0';
    } else if (strcmp(dir_arg, "-") == 0) {
        if (strlen(PREV_DIR) == 0) {
            fprintf(stderr, "No such directory!\n");
            return;
        }
        strcpy(target_path, PREV_DIR);
    } else {
        strcpy(target_path, dir_arg);
    }

// ############## LLM Generated Code Begins ##############

    struct dirent **namelist;
    int n = scandir(target_path, &namelist, NULL, alphasort);
    if (n < 0) {
        fprintf(stderr, "No such directory!\n");
        return;
    }
    
    for (int i = 0; i < n; i++) {
        if (!show_all && namelist[i]->d_name[0] == '.') {
            free(namelist[i]);
            continue;
        }
        printf("%s", namelist[i]->d_name);
        if (long_format) {
            printf("\n");
        } else {
            printf("  ");
        }
        free(namelist[i]);
    }
    if (!long_format && n > 0) printf("\n");
    free(namelist);
}

// ############## LLM Generated Code Ends ##############


// --- Function Dispatchers ---

int is_intrinsic(const char *command_name) {
    if (strcmp(command_name, "hop") == 0) return 1;
    if (strcmp(command_name, "cd") == 0) return 1;
    if (strcmp(command_name, "reveal") == 0) return 1;
    if (strcmp(command_name, "log") == 0) return 1;
    if (strcmp(command_name, "activities") == 0) return 1;
    if (strcmp(command_name, "ping") == 0) return 1;
    if (strcmp(command_name, "fg") == 0) return 1;
    if (strcmp(command_name, "bg") == 0) return 1;
    if (strcmp(command_name, "exit") == 0) return 1;
    return 0;
}

void execute_intrinsic(AtomicCommand *cmd) {
    if (strcmp(cmd->name, "hop") == 0 || strcmp(cmd->name, "cd") == 0) {
        handle_hop(cmd->args);
    } else if (strcmp(cmd->name, "reveal") == 0) {
        handle_reveal(cmd->args);
    } else if (strcmp(cmd->name, "log") == 0) {
        handle_log_command(cmd->args);
    } else if (strcmp(cmd->name, "activities") == 0) {
        handle_activities();
    } else if (strcmp(cmd->name, "ping") == 0) {
        handle_ping(cmd->args);
    } else if (strcmp(cmd->name, "fg") == 0) {
        handle_fg(cmd->args);
    } else if (strcmp(cmd->name, "bg") == 0) {
        handle_bg(cmd->args);
    } else if (strcmp(cmd->name, "exit") == 0) {
        kill_all_jobs();
        exit(0);
    }
}

