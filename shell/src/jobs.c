#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include "jobs.h"

#define MAX_JOBS 128

typedef struct {
    pid_t pid;
    char name[256];
    JobState state;
} Job;

static Job job_list[MAX_JOBS];
static int job_count = 0;

extern pid_t FOREGROUND_CHILD_PID;

void add_job(pid_t pid, const char *name, JobState state) {
    if (job_count < MAX_JOBS) {
        job_list[job_count].pid = pid;
        strncpy(job_list[job_count].name, name, sizeof(job_list[job_count].name) - 1);
        job_list[job_count].state = state;
        job_count++;
    }
}

void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (job_list[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                job_list[j] = job_list[j + 1];
            }
            job_count--;
            return;
        }
    }
}

// ############## LLM Generated Code Begins ##############

void check_background_processes() {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (job_list[i].pid == pid) {
                if (WIFEXITED(status)) {
                    printf("\n%s with pid %d exited normally\n", job_list[i].name, pid);
                    remove_job(pid);
                } else if (WIFSIGNALED(status)) {
                    printf("\n%s with pid %d exited abnormally\n", job_list[i].name, pid);
                    remove_job(pid);
                } else if (WIFSTOPPED(status)) {
                    job_list[i].state = STOPPED;
                    printf("\n[%d] Stopped %s\n", pid, job_list[i].name);
                }
                break;
            }
        }
    }
}

// ############## LLM Generated Code Ends ##############

void handle_activities() {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] : %s - %s\n", job_list[i].pid, job_list[i].name,
               job_list[i].state == RUNNING ? "Running" : "Stopped");
    }
}

void handle_ping(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "ping: Invalid syntax!\n");
        return;
    }
    pid_t pid = atoi(args[1]);
    int sig = atoi(args[2]);
    
    if (kill(pid, 0) == -1) {
        fprintf(stderr, "ping: No such process found\n");
        return;
    }
    
    if (kill(pid, sig % 32) == 0) {
        printf("Sent signal %d to process with pid %d\n", sig, pid);
    } else {
        perror("ping: kill failed");
    }
}

void handle_fg(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "fg: Job number required\n");
        return;
    }
    pid_t pid = atoi(args[1]);
    int job_idx = -1;
    for (int i = 0; i < job_count; i++) {
        if (job_list[i].pid == pid) {
            job_idx = i;
            break;
        }
    }

    if (job_idx == -1) {
        fprintf(stderr, "fg: No such job\n");
        return;
    }

    Job job = job_list[job_idx];
    remove_job(pid);

    if (job.state == STOPPED) {
        kill(pid, SIGCONT);
    }

    // ############## LLM Generated Code Begins ##############

    FOREGROUND_CHILD_PID = pid;
    int status;
    waitpid(pid, &status, WUNTRACED);

    if (WIFSTOPPED(status)) {
        add_job(pid, job.name, STOPPED);
        printf("\n[%d] Stopped %s\n", pid, job.name);
    }
    FOREGROUND_CHILD_PID = -1;

    // ############## LLM Generated Code Ends ##############
}

void handle_bg(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "bg: Job number required\n");
        return;
    }
    pid_t pid = atoi(args[1]);
    int job_idx = -1;
    for (int i = 0; i < job_count; i++) {
        if (job_list[i].pid == pid) {
            job_idx = i;
            break;
        }
    }

    if (job_idx == -1) {
        fprintf(stderr, "bg: No such job\n");
        return;
    }

    if (job_list[job_idx].state == RUNNING) {
        fprintf(stderr, "Job already running\n");
        return;
    }

    kill(pid, SIGCONT);
    job_list[job_idx].state = RUNNING;
}

void kill_all_jobs() {
    for (int i = 0; i < job_count; i++) {
        kill(job_list[i].pid, SIGKILL);
    }
}