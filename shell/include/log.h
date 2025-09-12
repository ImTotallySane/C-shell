#ifndef LOG_H
#define LOG_H

void init_log();
void add_to_log(const char *command);
int should_log_command(const char *command);
void handle_log_command(char **args);

#endif // LOG_H