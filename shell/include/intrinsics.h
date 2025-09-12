#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "parser.h"

int is_intrinsic(const char *command_name);
void execute_intrinsic(AtomicCommand *cmd);

#endif // INTRINSICS_H