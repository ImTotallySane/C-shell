#ifndef JOB_H
#define JOB_H

#include <sys/types.h>
#include <stdbool.h>

#define MAX_JOBS 20

typedef struct {
    pid_t pid;
    char command_name[1024];
    bool is_active;
    int job_number;
} BackgroundJob;

void init_job_list();
void add_background_job(pid_t pid, const char* command_name);
void check_completed_jobs();

#endif // JOB_H