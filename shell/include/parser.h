#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS 128

// Represents a single I/O redirection
typedef struct Redirect {
    char type;
    char *filename;
    struct Redirect *next;
} Redirect;

// Represents a single atomic command, e.g., "ls -l" or "grep foo"
typedef struct AtomicCommand {
    char *name;
    char *args[MAX_ARGS];
    // This now points to a linked list of all redirections for this command
    Redirect *redirects;
    struct AtomicCommand *next; // For piping: command1 | command2
} AtomicCommand;

// Represents a group of piped commands, e.g., "cat file | grep foo | wc -l"
typedef struct CommandGroup {
    AtomicCommand *head_atomic;
    struct CommandGroup *next; // For sequencing: group1 ; group2
    char separator; // ';' or '&'
} CommandGroup;

// Represents the entire command line input
typedef struct ShellCommand {
    CommandGroup *head_group;
    int is_background; // Final '&' at the end of the line
} ShellCommand;

// Note: Takes a non-const char* because strtok_r modifies the string
ShellCommand* parse_input(char *input);
void free_shell_command(ShellCommand *shell_cmd);

#endif // PARSER_H