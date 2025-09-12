#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

static void free_redirect_list(Redirect *head);

static void free_redirect_list(Redirect *head) {
    while (head) {
        Redirect *temp = head;
        head = head->next;
        free(temp->filename);
        free(temp);
    }
}

// ############## LLM Generated Code Begins ##############

static void free_atomic_command_list(AtomicCommand *head) {
    while (head) {
        AtomicCommand *temp = head;
        head = head->next;

        // CRITICAL FIX: Your old version had a massive memory leak.
        // It did not free the `strdup`'d arguments.
        // In this parser, `name` is also `args[0]`. All args are allocated.
        for (int i = 0; temp->args[i] != NULL; i++) {
            free(temp->args[i]);
        }

        free_redirect_list(temp->redirects);
        free(temp);
    }
}

// ############## LLM Generated Code Ends ##############

void free_shell_command(ShellCommand *shell_cmd) {
    if (!shell_cmd) return;
    CommandGroup *current_group = shell_cmd->head_group;
    while (current_group) {
        CommandGroup *temp_group = current_group;
        current_group = current_group->next;
        free_atomic_command_list(temp_group->head_atomic);
        free(temp_group);
    }
    free(shell_cmd);
}

// --- strtok_r Based Parser (Updated to build a redirect list) ---

// ############## LLM Generated Code Begins ##############

static AtomicCommand* create_atomic_command(char *name_token) {
    AtomicCommand *cmd = (AtomicCommand*)calloc(1, sizeof(AtomicCommand));
    if (!cmd) return NULL;
    
    cmd->name = strdup(name_token);
    cmd->args[0] = cmd->name;
    return cmd;
}

ShellCommand* parse_input(char *input) {
    char *input_copy = strdup(input);
    if (!input_copy) return NULL;

    char *validation_copy = strdup(input);
    if (!validation_copy) { free(input_copy); return NULL; }

    char *check_ptr = validation_copy + strlen(validation_copy) - 1;
    while(check_ptr >= validation_copy && isspace((unsigned char)*check_ptr)) {
        check_ptr--;
    }
    if (check_ptr >= validation_copy && *check_ptr == ';') {
        free(input_copy);
        free(validation_copy);
        return NULL;
    }
    free(validation_copy);

    ShellCommand *shell_cmd = (ShellCommand*)calloc(1, sizeof(ShellCommand));
    if (!shell_cmd) { free(input_copy); return NULL; }

    char *last_char = input_copy + strlen(input_copy) - 1;
    while(last_char >= input_copy && isspace((unsigned char)*last_char)) {
        last_char--;
    }
    if (*last_char == '&') {
        shell_cmd->is_background = 1;
        *last_char = '\0';
    }

    CommandGroup *current_group_tail = NULL;
    char *save_group;
    char *group_str = strtok_r(input_copy, ";", &save_group);

    while (group_str != NULL) {
        CommandGroup *new_group = (CommandGroup*)calloc(1, sizeof(CommandGroup));
        if (!new_group) { free_shell_command(shell_cmd); free(input_copy); return NULL; }

        AtomicCommand *current_atomic_tail = NULL;
        char *save_pipe;
        char *atomic_str = strtok_r(group_str, "|", &save_pipe);

        while (atomic_str != NULL) {
            char *save_token;
            char *token = strtok_r(atomic_str, " \t\r\n", &save_token);
            if (!token) goto syntax_error;

            AtomicCommand *new_atomic = create_atomic_command(token);
            int arg_count = 1;
            Redirect *redirect_tail = NULL;

            while ((token = strtok_r(NULL, " \t\r\n", &save_token)) != NULL) {
                char redirect_type = 0;
                if (strcmp(token, "<") == 0) redirect_type = '<';
                else if (strcmp(token, ">") == 0) redirect_type = '>';
                else if (strcmp(token, ">>") == 0) redirect_type = 'a';

                if (redirect_type) {
                    token = strtok_r(NULL, " \t\r\n", &save_token);
                    if (!token) goto syntax_error;

                    Redirect *new_redirect = (Redirect*)calloc(1, sizeof(Redirect));
                    if(!new_redirect) goto syntax_error;
                    new_redirect->type = redirect_type;
                    new_redirect->filename = strdup(token);

                    if (new_atomic->redirects == NULL) {
                        new_atomic->redirects = new_redirect;
                        redirect_tail = new_redirect;
                    } else {
                        redirect_tail->next = new_redirect;
                        redirect_tail = new_redirect;
                    }
                } else {
                    if (arg_count < MAX_ARGS - 1) {
                        new_atomic->args[arg_count++] = strdup(token);
                    }
                }
            }
            
            if (new_group->head_atomic == NULL) {
                new_group->head_atomic = new_atomic;
                current_atomic_tail = new_atomic;
            } else {
                current_atomic_tail->next = new_atomic;
                current_atomic_tail = new_atomic;
            }
            atomic_str = strtok_r(NULL, "|", &save_pipe);
        }

        if (new_group->head_atomic == NULL) {
            free(new_group);
            goto syntax_error;
        }

        if (shell_cmd->head_group == NULL) {
            shell_cmd->head_group = new_group;
            current_group_tail = new_group;
        } else {
            current_group_tail->next = new_group;
            current_group_tail = new_group;
        }
        current_group_tail->separator = ';'; // Assume ; for now
        group_str = strtok_r(NULL, ";", &save_group);
    }
    if (current_group_tail) current_group_tail->separator = '\0';

    free(input_copy);
    if (!shell_cmd->head_group) {
        free(shell_cmd);
        return NULL;
    }
    return shell_cmd;

syntax_error:
    free_shell_command(shell_cmd);
    free(input_copy);
    return NULL;
}

// ############## LLM Generated Code Ends ############## (had a bunch of trouble with manual parsing)