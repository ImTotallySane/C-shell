#ifndef TOKEN_H
#define TOKEN_H

typedef enum
    {
        name,
        token_pipe,
        ampersand,
        semicolon,
        input,
        output,
        append,
        eof
    } token_type;

typedef struct
    {
        char* name;
        token_type type;
    } token;

token* tokenize(char* user_input, int* count);

int find_token_count(token* list);

void free_tokens(token* list, int count);

#endif