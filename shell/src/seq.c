#include <stdlib.h>
#include "token.h"

#define MAX_SEQ_COMMANDS 20

/**
 * @brief Splits a flat list of tokens into a 2D array of command chunks.
 *
 * Each row in the returned 2D array points to the first token of a
 * semicolon-separated command. The last row is set to NULL.
 *
 * @param all_tokens The complete list of tokens from user input.
 * @param total_tokens The total number of tokens.
 * @return A dynamically allocated, NULL-terminated 2D array of token pointers.
 */
token** split_tokens_by_semicolon(token* all_tokens, int total_tokens) {
    // Allocate space for an array of pointers to tokens
    token** command_chunks = malloc(MAX_SEQ_COMMANDS * sizeof(token*));
    if (command_chunks == NULL) {
        return NULL;
    }

    int num_commands = 0;
    
    // The first command always starts at the beginning
    if (total_tokens > 1) {
        command_chunks[num_commands++] = &all_tokens[0];
    }

    // Find the start of subsequent commands
    for (int i = 0; i < total_tokens - 1; i++) {
        if (all_tokens[i].type == semicolon) {
            // The next token is the start of a new command
            if (i + 1 < total_tokens - 1) {
                command_chunks[num_commands++] = &all_tokens[i + 1];
            }
            // Replace the semicolon with an eof token to terminate the sub-list
            all_tokens[i].type = eof;
            all_tokens[i].name = NULL;
        }
    }

    // Terminate the main array with a NULL pointer
    command_chunks[num_commands] = NULL;

    return command_chunks;
}