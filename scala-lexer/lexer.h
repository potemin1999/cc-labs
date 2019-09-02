//
// Created by Ilya Potemin on 8/30/19.
//

#ifndef CC_LABS_LEXER_H
#define CC_LABS_LEXER_H

#include <stdint.h>

/// Identifier token
/// Contains char *ident_value
#define TOKEN_IDENTIFIER 1U

/// Keyword token
/// Contains uint32_t keyword
#define TOKEN_KEYWORD 2U

/// Bool literal
/// Contains bool_t bool_value
#define TOKEN_BOOL_LITERAL 8U

/// Integer literal
/// Contains int32_t int_value
#define TOKEN_INT_LITERAL 9U

/// Float literal
/// Contains float float_value
#define TOKEN_FLOAT_LITERAL 10U

/// String literal
/// Contains char *string_value;
#define TOKEN_STRING_LITERAL 11U

/// boolean type
typedef int bool_t;

/**
 * token_t represents language lexeme
 * Content of the region should be read with accordance with the token type
 *
 */
typedef struct {
    uint8_t type;
    union {
        uint32_t keyword;
        char *ident_value;
        bool_t bool_value;
        int32_t int_value;
        float float_value;
        char *string_value;
    };
} token_t;

token_t lex_next();

//TODO: add other keywords and fix numbers
#define KEYWORD_IF      0x00000001

#endif //CC_LABS_LEXER_H
