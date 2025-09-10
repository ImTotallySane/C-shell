#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "job.h"

static BackgroundJob job_list[MAX_JOBS];
static int next_job_number = 1;

void init_job_list() {
    for (int i = 0; i < MAX_JOBS; i++) {
        job_list[i].is_active = false;
    }
}

void add_background_job(pid_t pid, const char* command_name) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_list[i].is_active) {
            job_list[i].pid = pid;
            strncpy(job_list[i].command_name, command_name, sizeof(job_list[i].command_name) - 1);
            job_list[i].command_name[sizeof(job_list[i].command_name) - 1] = '\0';
            job_list[i].is_active = true;
            job_list[i].job_number = next_job_number++;
            printf("[%d] %d\n", job_list[i].job_number, pid);
            fflush(stdout);
            return;
        }
    }
    fprintf(stderr, "shell: Maximum number of background jobs reached.\n");
}

void check_completed_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_list[i].is_active) {
            int status;
            pid_t result = waitpid(job_list[i].pid, &status, WNOHANG);

            if (result == job_list[i].pid) {
                if (WIFEXITED(status)) {
                    printf("\n[%d]+ Done\t%s with pid %d exited normally\n", job_list[i].job_number, job_list[i].command_name, job_list[i].pid);
                } else {
                    printf("\n[%d]+ Done\t%s with pid %d exited abnormally\n", job_list[i].job_number, job_list[i].command_name, job_list[i].pid);
                }
                job_list[i].is_active = false;
            }
        }
    }
}