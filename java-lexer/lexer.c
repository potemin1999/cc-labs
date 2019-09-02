//
// Created by ilya on 8/30/19.
//

#include <stdio.h>
#include "lexer.h"

#define IS_LETTER(x) (((x)>='a' && ((x)<='z') || ((x)>='A' && (x)<='Z')) )
#define IS_UNDERSCORE(x) ((x)=='_')

#define BUFFER_SIZE 4096
char lex_buffer[BUFFER_SIZE];

int32_t symbols_left = 0;

char lex_next_symbol() {
    if (symbols_left == 0) {
        symbols_left = fread(lex_buffer, BUFFER_SIZE, 1, stdin);
        if (symbols_left == 0) {
            symbols_left = -1;
            return 0;
        }
    }
    return lex_buffer[BUFFER_SIZE - symbols_left];
}

token_t lex_next() {
    token_t token;
    char c1 = lex_next_symbol();
    if (IS_LETTER(c1) || c1 == '_' || c1 == '$') {
        // keyword or identifier

    }
    return 0;
}
