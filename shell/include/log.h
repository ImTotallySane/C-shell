#ifndef LOG_H
#define LOG_H

#include"token.h"

int load(FILE* ptr, char** commands);

void store(const char* input, char** commands, int command_count);

void print_commands(char** commands, int command_count);

#endif