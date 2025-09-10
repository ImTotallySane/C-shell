#ifndef PIPE_H
#define PIPE_H

#include "token.h"  // Assuming token.h is in the same directory
#include "redir.h" // Assuming command.h defines the Command struct

#define MAX_COMMANDS_IN_PIPELINE 20

/**
 * @brief A structure to hold a series of commands forming a pipeline.
 */
typedef struct {
    Command commands[MAX_COMMANDS_IN_PIPELINE];
    int num_commands;
} Pipeline;

/**
 * @brief Parses a raw token list into a Pipeline structure.
 * @param list The array of tokens from the tokenizer.
 * @param count The number of tokens.
 * @return A Pipeline struct populated with the parsed commands.
 */
Pipeline parse_tokens_for_pipeline(token* list, int count);

/**
 * @brief Executes a complete command pipeline.
 * @param pipeline The Pipeline struct containing the commands to execute.
 */
void execute_pipeline(Pipeline pipeline, char* homedir, char* prev_dir, bool has_previous_cwd);

#endif // PIPE_H