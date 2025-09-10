#ifndef HOP_H
#define HOP_H

#include "token.h"

// Executes the "hop" built-in command
// tokens       : array of parsed tokens
// token_count  : number of tokens in the array
// shell_home_dir : home directory path (usually set from $HOME)
//
// Returns 1 if directory was changed successfully, 0 otherwise.
int execute_hop(token* tokens, int token_count, const char* shell_home_dir);

#endif // HOP_H
