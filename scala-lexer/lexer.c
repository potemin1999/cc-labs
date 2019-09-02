//
// Created by Ilya Potemin on 8/30/19.
//

#include <stdio.h>
#include "lexer.h"

/// returns 1 if a == "[a-zA-Z]", zero otherwise
#define IS_LETTER(x) (((x)>='a' && ((x)<='z') || ((x)>='A' && (x)<='Z')) )

/// returns 1 if a == "_", zero otherwise
#define IS_UNDERSCORE(x) ((x)=='_')

/// read block size while fetching input stream to the buffer
#define IN_BUFFER_SIZE 4096

/// limits the maximum size of literals and identifiers
#define OUT_BUFFER_SIZE 256

/// input buffer
char lex_buffer[IN_BUFFER_SIZE];

/// output accumulation buffer
char out_buffer[OUT_BUFFER_SIZE];

/// indirectly points to the out_buffer position
int32_t symbols_left = 0;

/// current size of the accumulation buffer
int32_t accum_symbols_size = 0;


/**
 * On demand returns next symbol of the input stream via block reading of descriptor input data flow
 * Do not shift the stream current pointer (do --symbols_left if the symbol was taken from the stream)
 * @return next symbol of lexer input stream
 */
char lex_next_symbol() {
    if (symbols_left == 0) {
        symbols_left = fread(lex_buffer, IN_BUFFER_SIZE, 1, stdin);
        if (symbols_left == 0) {
            symbols_left = -1;
            return 0;
        }
    }
    return lex_buffer[IN_BUFFER_SIZE - symbols_left];
}


/**
 * On demand returns next token extracted from the input stream, char by char obtained via lex_next_symbol()
 * Requiring the next char of the input stream will return the next char after the last one of the token
 * Represents
 * @return
 */
token_t lex_next() {
    token_t token = {
            // non-initialized token type
            .type = 0,
            // initialize only ident_value since pointer
            // has equal or the most size in the union
            .ident_value = 0
    };
    char c1 = lex_next_symbol();
    if (IS_LETTER(c1) || c1 == '_' || c1 == '$') {
        // keyword or identifier

    }
    return token;
}
