#ifndef EXEC_H
#define EXEC_H

#include "token.h" // Depends on the token structure

/**
 * @brief Executes an external command by forking a new process.
 *
 * This function handles the fork-exec-wait pattern to run system commands
 * that are not built into the shell.
 *
 * @param list The array of tokens representing the command and its arguments.
 * @param count The number of tokens in the array.
 */
void execute_external_command(token* list, int count);

#endif // EXEC_H