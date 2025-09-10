#ifndef REVEAL_H
#define REVEAL_H

#include <stdbool.h>
#include "token.h"

/**
 * @brief Executes the 'reveal' command to list directory contents.
 * @param tokens The array of tokens from user input.
 * @param token_count The number of tokens.
 * @param shell_home_dir The path to the shell's home directory.
 * @param previous_cwd The path to the shell's last working directory.
 * @param has_previous_cwd A flag indicating if previous_cwd is valid.
 */
void execute_reveal(token* tokens, int token_count, const char* shell_home_dir, const char* previous_cwd, bool has_previous_cwd);

#endif // REVEAL_H