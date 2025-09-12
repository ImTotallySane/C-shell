#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/utsname.h>
#include <limits.h>
#include <pwd.h>
#include "prompt.h"

extern char SHELL_HOME_DIR[PATH_MAX];

void display_prompt() {
    
    char username[LOGIN_NAME_MAX];
    struct passwd *pw = getpwuid(getuid());
    strncpy(username, pw->pw_name, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0'; // Ensure null-termination

//    ############## LLM Generated Code Begins ##############

    struct utsname sys_info;
    if (uname(&sys_info) != 0) {
        strcpy(sys_info.nodename, "system");
    }

//    ############## LLM Generated Code Ends ##############
    
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return;
    }

    char display_path[PATH_MAX];
    if (strncmp(cwd, SHELL_HOME_DIR, strlen(SHELL_HOME_DIR)) == 0) {
        if (strlen(cwd) == strlen(SHELL_HOME_DIR)) {
            strcpy(display_path, "~");
        } else {
            snprintf(display_path, sizeof(display_path), "~%s", cwd + strlen(SHELL_HOME_DIR));
        }
    } else {
        strcpy(display_path, cwd);
    }

    printf("<%s@%s:%s> ", username, sys_info.nodename, display_path);
    fflush(stdout);
}
