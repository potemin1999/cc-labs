//
// Created by Ilya Potemin on 8/30/19.
//

#include <stdio.h>
#include <string.h>
#include <malloc.h>
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
        case TOKEN_EOF: {
            return strdup("<eof>");
        }
        default: {
            return strdup("<no-type>");
        }
    }
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
