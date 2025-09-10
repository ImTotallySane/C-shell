#ifndef REDIR_H
#define REDIR_H
#include "token.h"

typedef struct {
    char** argv;          // The command and its arguments (e.g., {"wc", "-l", NULL})
    char* input_file;     // The name of the file for input redirection
    char* output_file;
    bool append_output;
    // You can add fields for output redirection here later
} Command;

Command parse_tokens_for_execution(token* list, int count);

void execute_redirected_command(Command cmd);

#endif