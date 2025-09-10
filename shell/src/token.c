#include<stdio.h>
#include"token.h"
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

// kind of gpted

token* tokenize(char* user_input, int* count) {
    int capacity = 10;
    token* list = malloc(capacity * sizeof(token));
    if (list == NULL) {
        *count = 0;
        return NULL;
    }
    *count = 0;
    int i = 0;
    
    while(user_input[i] != '\0') {
        if (isspace(user_input[i])) {
            i++;
            continue;
        }

        if (*count >= capacity) {
            capacity *= 2;
            token* temp_list = realloc(list, capacity * sizeof(token));
            if (temp_list == NULL) {
                // Handle realloc failure
                free(list);
                *count = 0;
                return NULL;
            }
            list = temp_list;
        }

        token current_token = {.name = NULL};

        if (user_input[i] == '|') {
            current_token.type = token_pipe;
            i++;
        } else if (user_input[i] == '&') {
            current_token.type = ampersand;
            i++;
        } else if (user_input[i] == ';') {
            current_token.type = semicolon;
            i++;
        } else if (user_input[i] == '<') {
            current_token.type = input;
            i++;
        } else if (user_input[i] == '>') {
            if (user_input[i+1] == '>') {
                current_token.type = append;
                i += 2;
            } else {
                current_token.type = output;
                i++;
            }
        } else {
            int start = i;
            // Scan to find the end of the name
            while(user_input[i] != '\0' && !isspace(user_input[i]) && user_input[i] != '|' && user_input[i] != '&' && user_input[i] != ';' && user_input[i] != '<' && user_input[i] != '>') {
                i++;
            }
            int length = i - start;
            current_token.name = malloc(length + 1);
            if (current_token.name == NULL) {
                free(list);
                *count = 0;
                return NULL;
            }
            strncpy(current_token.name, &user_input[start], length);
            current_token.name[length] = '\0';
            current_token.type = name;
        }

        list[*count] = current_token;
        (*count)++;
    }
    
    // Add EOF token
    if (*count >= capacity) {
        token* temp_list = realloc(list, (*count + 1) * sizeof(token));
        if (temp_list == NULL) {
            free(list);
            *count = 0;
            return NULL;
        }
        list = temp_list;
    }
    list[*count].type = eof;
    list[*count].name = NULL;
    (*count)++;

    return list;
}

void free_tokens(token* list, int count)
    {
        for(int i=0;i<count;i++)
            {
                if(list[i].name!=NULL)
                    {
                        free(list[i].name);
                    }
            }
        free(list);
    }

int find_token_count(token* list)
    {
        int i=0;
        while(list[i].name!=NULL)
            {
                i++;
            }
        return i+1;
    }