#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

typedef enum {
    RUNNING,
    STOPPED
} JobState;

void check_background_processes();
void add_job(pid_t pid, const char *name, JobState state);
void remove_job(pid_t pid);
void update_job_state(pid_t pid, JobState state);
void handle_activities();
void handle_ping(char **args);
void handle_fg(char **args);
void handle_bg(char **args);
void kill_all_jobs();

#endif // JOBS_H