//
// Created by ilya on 8/30/19.
//

#ifndef CC_LABS_LEXER_H
#define CC_LABS_LEXER_H

#include <stdint.h>

#define TOKEN_IDENTIFIER
#define TOKEN_KEYWORD
#define TOKEN_BOOL_LITERAL
#define TOKEN_BYTE_LITERAL
#define TOKEN_INT_LITERAL
#define TOKEN_FLOAT_LITERAL
#define TOKEN_DOUBLE_LITERAL
#define TOKEN_CHAR_LITERAL
#define TOKEN_STRING_LITERAL

typedef struct {
    uint8_t type;
    union {
        uint32_t keyword;
        uint32_t operator;
        char *ident_value;
    };
} token_t;

token_t lex_next();

#endif //CC_LABS_LEXER_H
