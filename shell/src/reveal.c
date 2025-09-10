#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

#include "reveal.h"

static int compare_strings(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

void execute_reveal(token* tokens, int token_count, const char* shell_home_dir, const char* previous_cwd, bool has_previous_cwd) {
    bool show_all = false;
    bool long_format = false;
    char* target_arg = NULL;

    // 1. Parse flags and arguments (this part is unchanged)
    for (int i = 1; i < token_count - 1; i++) {
        if (tokens[i].name[0] == '-') {
            for (size_t j = 1; j < strlen(tokens[i].name); j++) {
                if (tokens[i].name[j] == 'a') show_all = true;
                if (tokens[i].name[j] == 'l') long_format = true;
            }
        } else {
            if (target_arg != NULL) {
                fprintf(stderr, "reveal: Invalid Syntax!\n");
                return;
            }
            target_arg = tokens[i].name;
        }
    }

    // 2. Determine the target directory path (UPDATED LOGIC)
    char target_dir[PATH_MAX];
    char original_dir[PATH_MAX];

    if (target_arg == NULL || strcmp(target_arg, ".") == 0) {
        // ✅ NEW: No argument, or '.', means list the Current Working Directory.
        if (getcwd(target_dir, sizeof(target_dir)) == NULL) {
            perror("reveal: getcwd");
            return;
        }
    } else if (strcmp(target_arg, "~") == 0) {
        // ✅ NEW: '~' is now handled separately.
        if (shell_home_dir == NULL || shell_home_dir[0] == '\0') {
            fprintf(stderr, "No such directory!\n");
            return;
        }
        strcpy(target_dir, shell_home_dir);
    } else if (strcmp(target_arg, "..") == 0) {
        if (getcwd(original_dir, sizeof(original_dir)) == NULL) {
            perror("reveal: getcwd");
            return;
        }
        if (chdir("..") != 0) {
            perror("reveal: chdir");
            return;
        }
        if (getcwd(target_dir, sizeof(target_dir)) == NULL) {
            perror("reveal: getcwd");
            chdir(original_dir);
            return;
        }
        chdir(original_dir);
    } else if (strcmp(target_arg, "-") == 0) {
        if (!has_previous_cwd) {
            fprintf(stderr, "No such directory!\n");
            return;
        }
        strcpy(target_dir, previous_cwd);
    } else {
        strcpy(target_dir, target_arg);
    }

    // Sections 3, 4, 5, and 6 for reading, sorting, printing,
    // and cleaning up are exactly the same as before.
    
    DIR* dir = opendir(target_dir);
    if (dir == NULL) {
        fprintf(stderr, "No such directory!\n");
        return;
    }
    
    struct dirent* entry;
    char** filenames = NULL;
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }
        filenames = realloc(filenames, (count + 1) * sizeof(char*));
        if (filenames == NULL) { perror("reveal: realloc"); closedir(dir); return; }
        filenames[count++] = strdup(entry->d_name);
    }
    closedir(dir);
    
    qsort(filenames, count, sizeof(char*), compare_strings);
    
    for (int i = 0; i < count; i++) {
        if (long_format) {
            printf("%s\n", filenames[i]);
        } else {
            printf("%s%s", filenames[i], (i == count - 1) ? "" : "  ");
        }
    }
    if (!long_format && count > 0) {
        printf("\n");
    }
    
    for (int i = 0; i < count; i++) {
        free(filenames[i]);
    }
    free(filenames);
}