#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include<sys/types.h>
#include "signals.h"

extern pid_t FOREGROUND_CHILD_PID;

void sigint_handler(int signo) {
    if (FOREGROUND_CHILD_PID > 0) {
        kill(FOREGROUND_CHILD_PID, SIGINT);
    }
    printf("\n"); // Newline after ^C
}

void sigtstp_handler(int signo) {
    if (FOREGROUND_CHILD_PID > 0) {
        kill(FOREGROUND_CHILD_PID, SIGTSTP);
    }
}

void setup_signal_handlers() {
    struct sigaction sa_int, sa_tstp;

    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);

    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);
}