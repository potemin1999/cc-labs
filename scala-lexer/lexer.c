//
// Created by Ilya Potemin on 8/30/19.
//

#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <wchar.h>
#include "lexer.h"

#define REPORT_ERROR_WITH_POS(str) {            \
    size_t str_len = strlen(str);               \
    char buffer[str_len + 32];                  \
    sprintf(buffer,"at position %d : %s",       \
            input_symbols_ptr,str);             \
    on_lex_error(buffer);                       \
    }

#define COMMIT() ++input_symbols_ptr;

#define COMMIT_AND_SHIFT(var_name)          \
    COMMIT()                                \
    symbol_t var_name = lex_next_symbol();

#define HEX_TO_INT(x) (x) < 58 ? (x) - 48 : \
    ((x) > 96 ? (x) - 87 : (x) - 55)

/// returns true, if character is a digit
#define IS_DIGIT(x) ((x)>='0' && (x)<='9')

// returns true, if character is a digit or [a-fA-F]
#define IS_HEX_DIGIT(x)         \
    (IS_DIGIT(x)) ||            \
    ((x)>='a' && (x<='f')) ||   \
    ((x)>='A' && (x<='F'))      \

///returns true, if character is an operator character
#define IS_OPER(x) (\
    ((x)=='+') ||   \
    ((x)=='-') ||   \
    ((x)=='*') ||   \
    ((x)=='/') ||   \
    ((x)=='%') ||   \
    ((x)=='=') ||   \
    ((x)=='!') ||   \
    ((x)=='>') ||   \
    ((x)=='<') ||   \
    ((x)=='&') ||   \
    ((x)=='|') ||   \
    ((x)=='^') ||   \
    ((x)=='~')      \
    )

/// returns 1 if a == "[a-zA-Z]", zero otherwise
#define IS_LETTER(x) (((x)>='a' && ((x)<='z') || ((x)>='A' && (x)<='Z')) )

/// returns 1 if a == '_', zero otherwise
#define IS_UNDERSCORE(x) ((x)=='_')

/// returns 1 if a == '`', zero otherwise
#define IS_BACKQUOTE(x) ((x)=='`')

/// read block size while fetching input stream to the buffer
#define IN_BUFFER_SIZE 4096

/// limits the maximum size of literals and identifiers
#define ACCUM_BUFFER_SIZE 256

FILE *input_file = 0;

/// input buffer
symbol_t lex_buffer[IN_BUFFER_SIZE];

/// output accumulation buffer
symbol_t initial_accum_buffer[ACCUM_BUFFER_SIZE];
symbol_t *accum_buffer = initial_accum_buffer;

/// count of read symbols in the buffer
int32_t input_symbols_size = 0;
/// points to the out_buffer position
int32_t input_symbols_ptr = 0;

/// current size of the accumulation buffer
int32_t accum_symbols_size = 0;
int32_t accum_symbols_cap = ACCUM_BUFFER_SIZE;


/**
 * On demand returns next symbol of the input stream via block reading of descriptor input data flow
 * Do not shift the stream current pointer (do --symbols_left if the symbol was taken from the stream)
 * @return next symbol of lexer input stream
 */
symbol_t lex_next_symbol() {
    if (input_file == 0) {
        input_file = stdin;
    }
    FILE *file = input_file;
    int a = (file == stdin);
    size_t input_ptr = input_symbols_ptr;
    size_t input_size = input_symbols_size;
    if (input_symbols_ptr >= input_symbols_size) {
        input_symbols_size = fread(lex_buffer, 1, IN_BUFFER_SIZE, input_file);
        if (input_symbols_size == 0) {
            int code = 0;
            if (code = ferror(input_file)) {
                printf("error %d occurred while reading\n", code);
            }
            input_symbols_size = 0;
            return '\0';
        }
        input_symbols_ptr = 0;
    }
    symbol_t ret = lex_buffer[input_symbols_ptr];
    return ret;
}

/**
 * Saves symbol to the accumulation buffer
 * @param symbol Symbol to save
 * @return new accum_symbols_size
 */
static inline int lex_accum_symbol(symbol_t symbol) {
    if (accum_symbols_size == accum_symbols_cap) {
        if (accum_symbols_size == ACCUM_BUFFER_SIZE) {
            accum_buffer = malloc(sizeof(symbol_t) * accum_symbols_cap * 2);
        } else {
            accum_buffer = realloc(accum_buffer, sizeof(symbol_t) * accum_symbols_cap * 2);
        }
    }
    accum_buffer[accum_symbols_size] = symbol;
    return ++accum_symbols_size;
}

/**
 * This function tries to build and operator sequence by given beginning
 * Is case to fetch more symbols it commits symbol read and calls lex_next_symbol()
 * @param first First symbol of operator to begin with
 * @param operator_out Pointer to the operator type variable
 * @return size of read operator in symbols
 */
size_t lex_build_operator(symbol_t first, uint32_t *operator_out);

int build_integer_literal(token_t *token, uint8_t is_hex);

int build_float_literal(token_t *token, uint8_t has_exponent, uint8_t is_double);

void lex_input(FILE *input_desc) {
    input_file = input_desc;
}

token_t lex_next() {
    // main function of the lexer
    token_t token = {
            // non-initialized token type
            .type = 0,
            // initialize only ident_value since pointer
            // has equal or the most size in the union
            .ident_value = 0
    };
    symbol_t c1 = lex_next_symbol();
    if (IS_BACKQUOTE(c1)) {
        // back quote starting identifier, read everything until next backquote
        // newlines are not allowed
        COMMIT()
        symbol_t next = lex_next_symbol();
        while (!IS_BACKQUOTE(next)) {
            COMMIT();
            if (next == '\n') { //newline is an error
                REPORT_ERROR_WITH_POS("newlines are not allowed in back-quoted identifiers")
                break;
            }
            if (next == '\0') { //eof is unexpected
                REPORT_ERROR_WITH_POS("eof while reading back-quoted identifier");
                break;
            }
            lex_accum_symbol(next);
            next = lex_next_symbol();
        }
        // check is the lexing was succeed
        if (IS_BACKQUOTE(next)) {
            COMMIT()
            size_t n_size = sizeof(token_t) * accum_symbols_size;
            char *ident_value = malloc(n_size);
            memcpy(ident_value, accum_buffer, n_size);
            bzero(accum_buffer, n_size);
            accum_symbols_size = 0;
            token.type = TOKEN_IDENTIFIER;
            token.ident_value = ident_value;
            return token;
        } else {
            return token;
        }
    }
    if (IS_OPER(c1)) {
        // operator identifier begins with operator character
        lex_accum_symbol(c1);
        COMMIT()
        symbol_t next = lex_next_symbol();
        while (IS_OPER(next)) {
            lex_accum_symbol(next);
            COMMIT()
            next = lex_next_symbol();
        }
        size_t n_size = sizeof(symbol_t) * accum_symbols_size;
        char *ident_value = malloc(n_size);
        memcpy(ident_value, accum_buffer, n_size);
        bzero(accum_buffer, n_size);
        accum_symbols_size = 0;
        token.type = TOKEN_IDENTIFIER;
        token.ident_value = ident_value;
        return token;
    }
    if (IS_LETTER(c1) || c1 == '_' || c1 == '$') {
        // keyword or identifier
    }
    if (IS_DIGIT(c1)) {
        // integer or float literal
        lex_accum_symbol(c1);
        COMMIT()
        goto skip_parse_float_literal;

        start_parse_float_literal:;
        // we have float literal
        // save it as string
        symbol_t s = lex_next_symbol();
        do {
            lex_accum_symbol(s);
            COMMIT()
            s = lex_next_symbol();
            // check all allowed symbols in this literal
        } while (IS_DIGIT(s) || s == '.' ||
                 s == 'e' || s == 'E' || s == '-' || s == '+' ||
                 s == 'F' || s == 'f' || s == 'D' || s == 'd');
        build_float_literal(&token, 0, 0);
        return token;

        skip_parse_float_literal:
        if (c1 == '0') {
            // check hex numeral case
            symbol_t c2 = lex_next_symbol();
            if (c2 == 'x' || c2 == 'X') {
                // integer hex literal for sure
                lex_accum_symbol(c2);
                COMMIT()
                symbol_t hex_num = lex_next_symbol();
                // first symbol after x|X should be hex literal
                if (!IS_HEX_DIGIT(hex_num)) {
                    REPORT_ERROR_WITH_POS("expected hex numeral")
                    return token;
                }
                // while we have hex numerals, process the input
                do {
                    lex_accum_symbol(hex_num);
                    COMMIT()
                    hex_num = lex_next_symbol();
                } while (IS_HEX_DIGIT(hex_num));
                // return token
                if (hex_num == 'l' || hex_num == 'L') {
                    // skip the l|L at the end
                    COMMIT()
                }
                build_integer_literal(&token, 1);
                return token;
            } else {
                if (IS_DIGIT(c2) || c2 == '.' ||
                    c2 == 'E' || c2 == 'e') {
                    lex_accum_symbol(c2);
                    COMMIT()
                    goto start_parse_float_literal;
                }
                if (c2 == 'l' || c2 == 'L') {
                    COMMIT()
                }
                build_integer_literal(&token, 0);
                return token;
            }
        } else {
            // first digit is not a zero, we can accept any non-hex digits further
            symbol_t hex_num = lex_next_symbol();
            do {
                lex_accum_symbol(hex_num);
                COMMIT()
                hex_num = lex_next_symbol();
                if (IS_DIGIT(hex_num)) {
                    continue;
                }
            } while (IS_DIGIT(hex_num));

            if (hex_num == 'l' || hex_num == 'L') {
                COMMIT()
            } else {
                // we received symbol, which can be found in float literal only (or not)
                goto start_parse_float_literal;
            }
            build_integer_literal(&token, 0);
            return token;
        }
    }
    if (c1 == '\'') {
        // character literal is expected
        COMMIT()
        symbol_t c2 = lex_next_symbol();
        COMMIT()
        if (c2 == '\\') {
            // escape or unicode symbol
            symbol_t c3 = lex_next_symbol();
            if (c3 == 'u') {
                // unicode symbol
                COMMIT_AND_SHIFT(u1)
                COMMIT_AND_SHIFT(u2)
                COMMIT_AND_SHIFT(u3)
                COMMIT_AND_SHIFT(u4)
                uint8_t u1_val = HEX_TO_INT(u1);
                uint8_t u2_val = HEX_TO_INT(u2);
                uint8_t u3_val = HEX_TO_INT(u3);
                uint8_t u4_val = HEX_TO_INT(u4);
                token.char_value =
                        (u1_val << 12U) +
                        (u2_val << 8U) +
                        (u3_val << 4U) +
                        (u4_val);
                COMMIT()
            } else {
                COMMIT()
                symbol_t *s = (symbol_t *) &token.char_value;
                *s = '\\';
                *(s + 1) = c3;
            }
        } else {
            token.char_value = c2;
        }
        symbol_t last = lex_next_symbol();
        if (last != '\'') {
            REPORT_ERROR_WITH_POS(" closing single quote expected")
            return token;
        }
        COMMIT()
        token.type = TOKEN_CHAR_LITERAL;
        return token;
    }
    if (c1 == '\0') {
        token.type = TOKEN_EOF;
    }
    return token;
}

char *token_to_string(token_t *token) {
    switch (token->type) {
        case TOKEN_IDENTIFIER: {
            char *ident = token->ident_value;
            size_t ident_len = strlen(ident);
            char *buffer = malloc(ident_len + 16);
            sprintf(buffer, "<ident=%s>", ident);
            return buffer;
        }
        case TOKEN_INT_LITERAL: {
            uint32_t value = token->int_value;
            char *buffer = malloc(32);
            sprintf(buffer, "<literal(integer)=%d>", value);
            return buffer;
        }
        case TOKEN_FLOAT_LITERAL: {
            char *float_val = token->float_value;
            size_t float_len = strlen(float_val);
            char *buffer = malloc(float_len + 32);
            sprintf(buffer, "<literal(float)=%s>", float_val);
            return buffer;
        }
        case TOKEN_CHAR_LITERAL: {
            uint32_t value = token->char_value;
            char *buffer = malloc(32);
            uint8_t *value_ptr = &value;
            if (value > 256) {
                if (value_ptr[0] == '\\') {
                    sprintf(buffer, "<literal(char|escape)=\\%c>", value_ptr[1]);
                } else {
                    setlocale(LC_ALL, "");
                    wchar_t wchar = (wchar_t) value_ptr + 2;
                    sprintf(buffer, "<literal(char|unicode)=%lc>", wchar);
                }
            } else {
                sprintf(buffer, "<literal(char)=%c>", value);
            }
            return buffer;
        }
        case TOKEN_EOF: {
            return strdup("<eof>");
        }
        default: {
            return strdup("<no-type>");
        }
    }
}

int build_integer_literal(token_t *token, uint8_t is_hex) {
    size_t current = accum_symbols_size - 1;
    uint32_t value = 0;
    uint32_t mul = 1;
    int32_t end = is_hex ? 1 : -1;
    uint32_t mul_mul = is_hex ? 16 : 10;
    // read until the x|X expected at position 1
    while (current > end) {
        symbol_t x = accum_buffer[current];
        int x_int = x;
        uint32_t inc = HEX_TO_INT(x);
        value += inc * mul;
        mul *= mul_mul;
        --current;
    }
    token->type = TOKEN_INT_LITERAL;
    token->int_value = value;
    return 0;
}

int build_float_literal(token_t *token, uint8_t has_exponent, uint8_t is_double) {
    size_t n_size = sizeof(symbol_t) * accum_symbols_size;
    char *float_value = malloc(n_size);
    memcpy(float_value, accum_buffer, n_size);
    bzero(accum_buffer, n_size);
    accum_symbols_size = 0;
    token->type = TOKEN_FLOAT_LITERAL;
    token->float_value = float_value;
    return 0;
}

#define WRITE_OP_AND_RET(size, op) { *operator_out = op; return size; }

size_t lex_build_operator(symbol_t first, uint32_t *operator_out) {
    switch (first) {
        case '+': {             // +,+=
            COMMIT_AND_SHIFT(s2)
            if (s2 == '=') {    // +=
                COMMIT()
                WRITE_OP_AND_RET(2, OP_ADD_ASSIGN)
            } else {            // +
                WRITE_OP_AND_RET(1, OP_ADD)
            }
        }
        case '-': {             // -,-=
            COMMIT_AND_SHIFT(s2)
            if (s2 == '=') {    // -=
                COMMIT()
                WRITE_OP_AND_RET(2, OP_SUB_ASSIGN)
            } else {            // -
                WRITE_OP_AND_RET(1, OP_SUB)
            }
        }
        case '*': {             //*, **, *=, **=
            COMMIT_AND_SHIFT(s2)
            if (s2 == '*') {    // **, **=
                COMMIT_AND_SHIFT(s3)
                if (s3 == '=') {// **=
                    COMMIT()
                    WRITE_OP_AND_RET(3, OP_EXP_ASSIGN)
                } else {        // **
                    WRITE_OP_AND_RET(2, OP_EXP)
                }
            } else if (s2 == '=') { // *=
                COMMIT()
                WRITE_OP_AND_RET(2, OP_MULT_ASSIGN)
            } else {            // *
                WRITE_OP_AND_RET(1, OP_MULT)
            }
        }
        case '/': {             // /, /=
            COMMIT_AND_SHIFT(s2)
            if (s2 == '=') {    // /=
                COMMIT()
                WRITE_OP_AND_RET(2, OP_DIV_ASSIGN)
            } else {            // /
                WRITE_OP_AND_RET(1, OP_DIV)
            }
        }
        case '%': {             // %, %=
            COMMIT_AND_SHIFT(s2)
            if (s2 == '=') {    // %=
                COMMIT()
                WRITE_OP_AND_RET(2, OP_MOD_ASSIGN)
            } else {            // %
                WRITE_OP_AND_RET(1, OP_MOD)
            }
        }
        case '=': {             // ==,=
            COMMIT_AND_SHIFT(s2)
            if (s2 == '=') {    // ==
                COMMIT()
                WRITE_OP_AND_RET(2, OP_EQ_TO)
            } else {            // =
                WRITE_OP_AND_RET(1, OP_ASSIGN)
            }
        }
        case '!': {             // !=, !
            COMMIT_AND_SHIFT(s2)
            if (s2 == '=') {    // !=
                COMMIT()
                WRITE_OP_AND_RET(2, OP_NEQ_TO)
            } else {            // !
                WRITE_OP_AND_RET(1, OP_L_NOT)
            }
        }
        case '>': {             // >, >=, >>, >>>, >>=
            COMMIT_AND_SHIFT(s2)
            if (s2 == '>') {    // >>, >>>, >>=
                COMMIT_AND_SHIFT(s3)
                if (s3 == '>') {// >>>
                    COMMIT()
                    WRITE_OP_AND_RET(3, OP_RSH_Z)
                } else if (s3 == '=') { // >>=
                    COMMIT()
                    WRITE_OP_AND_RET(3, OP_RSH_ASSIGN)
                } else {
                    WRITE_OP_AND_RET(2, OP_RSH)
                }
            } else if (s2 == '=') {// >=
                COMMIT()
                WRITE_OP_AND_RET(2, OP_GT_THAN_EQ_TO)
            } else {             // >
                WRITE_OP_AND_RET(1, OP_GT_THAN)
            }
        }
        case '<': {             // <,<=, <<, <<=
            COMMIT_AND_SHIFT(s2)
            if (s2 == '<') {    // <<, <<=
                COMMIT_AND_SHIFT(s3)
                if (s3 == '=') { // <<=
                    COMMIT()
                    WRITE_OP_AND_RET(3, OP_LSH_ASSIGN)
                } else {
                    WRITE_OP_AND_RET(2, OP_LSH)
                }
            } else if (s2 == '=') {// <=
                COMMIT()
                WRITE_OP_AND_RET(2, OP_LS_THAN_EQ_TO)
            } else {            // >
                WRITE_OP_AND_RET(1, OP_LS_THAN)
            }
        }
        case '&': {             // &&, &=, &
            COMMIT_AND_SHIFT(s2)
            if (s2 == '&') {    // &&
                COMMIT()
                WRITE_OP_AND_RET(2, OP_L_AND)
            } else if (s2 == '=') {// &=
                COMMIT()
                WRITE_OP_AND_RET(2, OP_B_AND_ASSIGN)
            } else {            // &
                WRITE_OP_AND_RET(1, OP_B_AND)
            }
        }
        case '|': {
            // ||, |=, |
            COMMIT_AND_SHIFT(s2)
            if (s2 == '|') {    // ||
                COMMIT()
                WRITE_OP_AND_RET(2, OP_L_OR)
            } else if (s2 == '=') {// |=
                COMMIT()
                WRITE_OP_AND_RET(2, OP_B_OR_ASSIGN)
            } else {            // |
                WRITE_OP_AND_RET(1, OP_B_OR)
            }
        }
        case '^': {             // ^=, ^
            COMMIT_AND_SHIFT(s2)
            if (s2 == '=') {    // ^=
                COMMIT()
                WRITE_OP_AND_RET(2, OP_B_XOR_ASSIGN)
            } else {
                WRITE_OP_AND_RET(1, OP_B_XOR)
            }
        }
        case '~': {             // ~
            COMMIT()
            WRITE_OP_AND_RET(1, OP_COMPL)
        }
        default: {
            WRITE_OP_AND_RET(0, 0)
        }
    }
}
