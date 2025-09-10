#include <stdio.h>
#include <string.h>
#include"log.h"

/**
 * @brief Loads commands from an open file into a pre-allocated array.
 * @param ptr A valid file pointer to the history file, opened for reading.
 * @param commands A 2D array of characters to store the commands.
 */
int load(FILE* ptr, char** commands) {
    int i = 0;
    const int max_commands = 15;
    const int buffer_size = 1000;

    // Loop until we either fill the array or reach the end of the file.
    // fgets() returns NULL when there are no more lines to read.
    while (i < max_commands && fgets(commands[i], buffer_size, ptr) != NULL) {
        // fgets() includes the newline character ('\n') at the end of the string.
        // This line finds that newline and replaces it with a null terminator ('\0').
        commands[i][strcspn(commands[i], "\n")] = '\0';

        // Move to the next line in the array
        i++;
    }
    return i;
}

void store(const char* input, char** commands, int command_count) {
    // Copy the new command from 'input' into the specified slot.
    // e.g., if command_count is 5, this becomes strcpy(commands[5], input);
    strcpy(commands[command_count], input);
}

void print_commands(char** commands, int command_count)
    {
        for(int i=0;i<command_count;i++)
            {
                printf("%s", commands[i]);
            }
    }