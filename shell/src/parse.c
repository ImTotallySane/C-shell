#include <stdio.h>
#include <stdbool.h>
#include "parse.h"
#include "token.h"

// --- Parser State ---
// These static variables hold the state of the parser during a single parse operation.
static token* g_tokens;      // The list of tokens we are parsing
static int g_current_pos;    // Our current position in the token list
static int g_token_count;    // Total number of tokens

// --- Forward Declarations for Parsing Functions ---
// Each function corresponds to a non-terminal in the grammar.
static bool parse_shell_cmd();
static bool parse_cmd_group();
static bool parse_atomic();

// --- Helper Functions ---

/**
 * @brief Returns the current token without consuming it.
 */
static token current_token() {
    return g_tokens[g_current_pos];
}

/**
 * @brief Consumes the current token and moves to the next one.
 */
static void advance() {
    // We don't advance past the 'eof' token.
    if (g_tokens[g_current_pos].type != eof) {
        g_current_pos++;
    }
}

// --- Grammar Implementation ---

/**
 * @brief Parses the 'atomic' rule.
 * atomic -> name (name | input | output)*
 */
static bool parse_atomic() {
    // An atomic command MUST start with a name (e.g., 'ls', 'cat').
    if (current_token().type != name) {
        return false;
    }
    advance(); // Consume the command name.

    // After the name, we can have a sequence of arguments or redirections.
    while (true) {
        token_type type = current_token().type;

        if (type == name) {
            // It's an argument.
            advance();
        } else if (type == input || type == output || type == append) {
            // It's a redirection operator (<, >, >>).
            advance(); // Consume the operator.
            // A redirection operator MUST be followed by a 'name' (the filename).
            if (current_token().type != name) {
                return false;
            }
            advance(); // Consume the filename.
        } else {
            // If it's not a name or redirection, this atomic command is done.
            break;
        }
    }
    return true;
}

/**
 * @brief Parses the 'cmd_group' rule.
 * cmd_group -> atomic (| atomic)*
 */
static bool parse_cmd_group() {
    // A command group must start with an atomic command.
    if (!parse_atomic()) {
        return false;
    }

    // It can then be followed by zero or more pipes, each followed by another atomic command.
    while (current_token().type == token_pipe) {
        advance(); // Consume the '|'.
        // After a pipe, there MUST be another atomic command.
        if (!parse_atomic()) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Parses the 'shell_cmd' rule (the top-level rule).
 * shell_cmd -> cmd_group ((& | ;) cmd_group)* &?
 */
static bool parse_shell_cmd() {
    // A shell command must start with a command group.
    if (!parse_cmd_group()) {
        return false;
    }

    // It can then be followed by zero or more separators, each followed by another command group.
    while (current_token().type == ampersand || current_token().type == semicolon) {
        advance(); // Consume the '&' or ';'.

        // Check for an optional trailing ampersand. If we see '&' followed by 'eof',
        // it's a background command at the end of the line, which is valid.
        if (g_tokens[g_current_pos - 1].type == ampersand && current_token().type == eof) {
            break;
        }

        // Otherwise, after a separator, there MUST be another command group.
        if (!parse_cmd_group()) {
            return false;
        }
    }

    // Handle the optional trailing ampersand.
    if (current_token().type == ampersand) {
        advance(); // Consume the final '&'.
    }

    // After a valid command, we must be at the end of the input.
    if (current_token().type != eof) {
        return false;
    }

    return true;
}


// --- Public Interface ---

/**
 * @brief Main parsing function. Initializes the parser state and starts the process.
 */
bool parse(token* tokens, int token_count) {
    if (tokens == NULL || token_count == 0) {
        return false; // Cannot parse nothing.
    }
    
    // Handle empty input (e.g., user just presses Enter). This is valid.
    if (token_count == 1 && tokens[0].type == eof) {
        return true;
    }

    // Initialize global parser state.
    g_tokens = tokens;
    g_current_pos = 0;
    g_token_count = token_count;

    // Start parsing from the top-level grammar rule.
    if (parse_shell_cmd()) {
        return true; // Success!
    } else {
        printf("Invalid Syntax!\n");
        return false; // Failure.
    }
}