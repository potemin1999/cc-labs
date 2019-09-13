/**
 * Scala Lexer
 *
 * @author Ilya Potemin
 * @author Anton Antonov
 * @author Abdulkhamid Muminov
 */

#include <clocale>
#include <cstring>
#include <malloc.h>
#include <cstdlib>
#include "lexer.h"

#define REPORT_ERROR_WITH_POS(str) {            \
    size_t str_len = strlen(str);               \
    char buffer[str_len + 32];                  \
    sprintf(buffer,"at position %d : %s",       \
            input_symbols_ptr,str);             \
        on_lex_error(buffer);                       \
    }

#define COMMIT() ++input_symbols_ptr;

#define COMMENT_CHECK(c1)                   \
    comment_skipping(c1);                   \
    c1 = lex_next_symbol();

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

/// returns 1 if a == '`', zero otherwise
#define IS_BACKQUOTE(x) ((x)=='`')

/// read block size while fetching input stream to the buffer
#define IN_BUFFER_SIZE 4096

/// limits the maximum size of literals and identifiers
#define ACCUM_BUFFER_SIZE 256

FILE *input_file = nullptr;

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

/// Line number and offset calculation required variables
int new_lines_num = 0;
int last_new_line_pos = -1;
int prev_new_line_pos = -1;


bool isKeyword() {
    char keywords[50][10] = {"abstract", "case", "catch", "class", "def",
                             "do", "else", "extends", "false", "final",
                             "finally", "for", "forSome", "if", "implicit",
                             "import", "lazy", "macro", "match", "new",
                             "null", "object", "override", "package", "private",
                             "protected", "return", "sealed", "super", "this",
                             "throw", "trait", "try", "true", "type",
                             "val", "var", "while", "with", "yield",
                             "_", ":", "=", "=>", "<-", "<:", "<%", ">:", "#", "@"};
    for (int i = 0; i < 50; ++i) {
        if (strcmp(keywords[i], accum_buffer) == 0) {
            return true;
        }
    }
    return false;
}


/**
 * On demand returns next symbol of the input stream via block reading of descriptor input data flow
 * Do not shift the stream current pointer (do --symbols_left if the symbol was taken from the stream)
 * @return next symbol of lexer input stream
 */
symbol_t lex_next_symbol() {
    if (input_file == nullptr) {
        input_file = stdin;
    }
    if (input_symbols_ptr >= input_symbols_size) {
        input_symbols_size = fread(lex_buffer, 1, IN_BUFFER_SIZE, input_file);
        if (input_symbols_size == 0) {
            int code = ferror(input_file);
            if (code) {
                printf("error %d occurred while reading\n", code);
            }
            input_symbols_size = 0;
            return '\0';
        }
        input_symbols_ptr = 0;
    }
    if ((char) lex_buffer[input_symbols_ptr] == '\n' && input_symbols_ptr != last_new_line_pos) {
        new_lines_num++;
        prev_new_line_pos = last_new_line_pos;
        last_new_line_pos = input_symbols_ptr;
    }
    symbol_t ret = lex_buffer[input_symbols_ptr];
    return ret;
}

symbol_t peek() {
    if (input_file == nullptr) {
        input_file = stdin;
    }
    if (input_symbols_ptr >= input_symbols_size) {
        input_symbols_size = fread(lex_buffer, 1, IN_BUFFER_SIZE, input_file);
        if (input_symbols_size == 0) {
            int code = 0;
            code = ferror(input_file);
            if (code) {
                printf("error %d occurred while reading\n", code);
            }
            input_symbols_size = 0;
            return '\0';
        }
        input_symbols_ptr = 0;
    }
    symbol_t ret = lex_buffer[input_symbols_ptr + 1];
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
            accum_buffer = (symbol_t *) malloc(sizeof(symbol_t) * accum_symbols_cap * 2);
        } else {
            accum_buffer = (symbol_t *) realloc(accum_buffer, sizeof(symbol_t) * accum_symbols_cap * 2);
        }
    }
    accum_buffer[accum_symbols_size] = symbol;
    return ++accum_symbols_size;
}

int build_integer_literal(token_t *token, uint8_t is_hex);

int build_float_literal(token_t *token, uint8_t is_double);

int build_string_literal(token_t *token, uint8_t has_trailing_quotes);

void lex_input(FILE *input_desc) {
    input_file = input_desc;
}

void comment_skipping(symbol_t c1) {
    if (c1 == '/') {
        if (peek() != '/' && peek() != '*')
            return;
        COMMIT()
        c1 = lex_next_symbol();
        COMMIT()
        if (c1 == '/') {
            //this is a comment
            while (c1 != '\n' && c1 != '\0') {
                c1 = lex_next_symbol();
                COMMIT()
            }
        } else if (c1 == '*') {

            c1 = lex_next_symbol();
            COMMIT()
            bool ok = false;
            while (lex_next_symbol() != '\0') {
                if (c1 == '*' && lex_next_symbol() == '/') {
                    COMMIT()
                    ok = true;
                    break;
                }
                c1 = lex_next_symbol();
                if (lex_next_symbol() != '\0')
                    COMMIT()

            }
            if (!ok) {
                REPORT_ERROR_WITH_POS("Compilation ERROR. Comment */ is not closed")
                exit(0);
            }

        }
    }

    if (lex_next_symbol() == '/') comment_skipping(lex_next_symbol());
}

token_t lex_next() {
    // main function of the lexer
    token_t token;
    // non-initialized token type
    token.type = 0;
    // initialize only ident_value since pointer
    // has equal or the most size in the union
    token.ident_value = nullptr;
    symbol_t c1 = lex_next_symbol();

    // retrieving current line number and offset and adding to the token
    // int curr_line = new_lines_num + 1;
    // int curr_offset = input_symbols_ptr - last_new_line_pos + 1;
    while ((c1 = lex_next_symbol()) == ' ') {
        COMMIT()
    }

    COMMENT_CHECK(c1)

    if (last_new_line_pos == input_symbols_ptr) {
        token.line = new_lines_num;
        token.offset = input_symbols_ptr - prev_new_line_pos;
    } else {
        token.line = new_lines_num + 1;
        token.offset = input_symbols_ptr - last_new_line_pos;
    }

    if (IS_BACKQUOTE(c1)) {
        // back quote starting identifier, read everything until next backquote
        // newlines are not allowed
        COMMIT()
        symbol_t next = lex_next_symbol();
        while (!IS_BACKQUOTE(next)) {
            COMMIT()
            if (next == '\n') { //newline is an error
                REPORT_ERROR_WITH_POS("newlines are not allowed in back-quoted identifiers")
                break;
            }
            if (next == '\0') { //eof is unexpected
                REPORT_ERROR_WITH_POS("eof while reading back-quoted identifier")
                break;
            }
            lex_accum_symbol(next);
            next = lex_next_symbol();
        }
        // check is the lexing was succeed
        if (IS_BACKQUOTE(next)) {
            COMMIT()
            size_t n_size = sizeof(token_t) * accum_symbols_size;
            char *ident_value = new char[n_size + 1];
            bzero(ident_value, n_size + 1);
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
        char *ident_value = new char[n_size + 1];
        bzero(ident_value, n_size + 1);
        memcpy(ident_value, accum_buffer, n_size);
        bzero(accum_buffer, n_size);
        accum_symbols_size = 0;
        token.type = TOKEN_IDENTIFIER;
        token.ident_value = ident_value;
        return token;
    }
    if (IS_LETTER(c1) || c1 == '_' || c1 == '$') {
        // keyword or identifier
        lex_accum_symbol(c1);
        COMMIT()
        symbol_t next = lex_next_symbol();
        while (IS_LETTER(next) || IS_DIGIT(next) ||
               next == '_' || next == '$') {
            lex_accum_symbol(next);
            COMMIT()
            next = lex_next_symbol();
        }
        size_t n_size = sizeof(symbol_t) * accum_symbols_size;
        char *str_value = new char[n_size + 1];
        bzero(str_value, n_size + 1);
        memcpy(str_value, accum_buffer, n_size);
        if (isKeyword()) {
            token.type = TOKEN_KEYWORD;
        } else {
            token.type = TOKEN_IDENTIFIER;
        }
        bzero(accum_buffer, n_size);
        accum_symbols_size = 0;
        token.ident_value = str_value;
        return token;
    }
    if (IS_DIGIT(c1)) {
        // integer or float literal
        lex_accum_symbol(c1);
        COMMIT()
        symbol_t s;
        goto skip_parse_float_literal;

        start_parse_float_literal:;
        // we have float literal
        // save it as string
        s = lex_next_symbol();
        do {
            lex_accum_symbol(s);
            COMMIT()
            s = lex_next_symbol();
            // check all allowed symbols in this literal
        } while (IS_DIGIT(s) || s == '.' ||
                 s == 'e' || s == 'E' || s == '-' || s == '+' ||
                 s == 'F' || s == 'f' || s == 'D' || s == 'd');
        build_float_literal(&token, 0);
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
            } while (IS_DIGIT(hex_num));

            if (hex_num == '.' || hex_num == 'E' || hex_num == 'e') {
                lex_accum_symbol(hex_num);
                COMMIT()
                goto start_parse_float_literal;
            }
            if (hex_num == 'l' || hex_num == 'L') {
                COMMIT()
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
                auto s = (symbol_t *) &token.char_value;
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
    if (c1 == '"') {
        // string literal starting
        COMMIT()
        symbol_t s = lex_next_symbol();
        if (s == '"') {
            // empty string or a multiline literal
            COMMIT_AND_SHIFT(s2)
            if (s2 == '"') {
                // multiline literal
                COMMIT()
                s = lex_next_symbol();
                int counter = 0;
                do {
                    lex_accum_symbol(s);
                    if (s == '"') {
                        ++counter;
                    } else {
                        counter = 0;
                    }
                    COMMIT()
                    s = lex_next_symbol();
                } while (counter < 3);
                build_string_literal(&token, 1);
            } else {
                // empty string
                build_string_literal(&token, 0);
            }
        } else {
            symbol_t prev = 0;
            do {
                lex_accum_symbol(s);
                COMMIT()
                prev = s;
                s = lex_next_symbol();
                if (s == '"' && prev != '\\') {
                    break;
                }
            } while (s != 0);
            COMMIT()
            build_string_literal(&token, 0);
        }
        return token;
    }
    if (c1 == '\n' || c1 == ';') {
        token.type = TOKEN_DELIMITER;
        token.delim = DELIM_NEWLINE;
        COMMIT()
        while (lex_next_symbol() == '\n') {
            COMMIT()
        }
        return token;
    }
    if (c1 == '{' || c1 == '}') {
        token.type = TOKEN_DELIMITER;
        token.delim = (c1 == '{' ? DELIM_BRACE_OPEN : DELIM_BRACE_CLOSE);
        COMMIT()
        return token;
    }
    if (c1 == '[' || c1 == ']') {
        token.type = TOKEN_DELIMITER;
        token.delim = (c1 == '[' ? DELIM_BRACKET_OPEN : DELIM_BRACKET_CLOSE);
        COMMIT()
        return token;
    }
    if (c1 == '(' || c1 == ')') {
        token.type = TOKEN_DELIMITER;
        token.delim = (c1 == '(' ? DELIM_PARENTESIS_OPEN : DELIM_PARENTESIS_CLOSE);
        COMMIT()
        return token;
    }

    if (c1 == '.') {
        token.type = TOKEN_DELIMITER;
        token.delim = DELIM_DOT;
        COMMIT()
        return token;
    }
    if (c1 == ',') {
        token.type = TOKEN_DELIMITER;
        token.delim = DELIM_COMMA;
        COMMIT()
        return token;
    }
    if (c1 == ':') {
        token.type = TOKEN_DELIMITER;
        token.delim = DELIM_COLON;
        COMMIT()
        return token;
    }

    if (c1 == '\0') {
        token.type = TOKEN_EOF;
    }
    return token;
}

char *token_to_string(token_t *token) {
    char *buffer = nullptr;
    // Additional data to add
    // char* to_add 

    char *token_name, *token_val;

    switch (token->type) {
        case TOKEN_IDENTIFIER: {
            char *ident = token->ident_value;
            token_name = strdup("ident");
            token_val = strdup(ident);
            break;
        }
        case TOKEN_KEYWORD: {
            char *kw = token->ident_value;
            token_name = strdup("keyword");
            token_val = strdup(kw);
            break;
        }
        case TOKEN_INT_LITERAL: {
            uint32_t value = token->int_value;
            token_name = strdup("literal(integer)");
            token_val = new char[256];
            sprintf(token_val, "%d", value);
            break;
        }
        case TOKEN_FLOAT_LITERAL: {
            char *float_val = token->float_value;
            token_name = strdup("literal(float)");
            token_val = strdup(float_val);
            break;
        }
        case TOKEN_CHAR_LITERAL: {
            uint32_t value = token->char_value;
            auto *value_ptr = (uint8_t *) &value;
            if (value > 256) {
                if (value_ptr[0] == '\\') {
                    token_name = strdup("literal(char|escape)");
                    token_val = new char[256];
                    sprintf(token_val, "%c", value_ptr[1]);
                } else {
                    setlocale(LC_ALL, "");
                    auto wchar = (wchar_t *) (value_ptr + 2);
                    token_name = strdup("literal(char|unicode)");
                    token_val = new char[256];
                    sprintf(token_val, "%lc", *wchar);
                }
            } else {
                token_name = strdup("literal(char)");
                token_val = new char[256];
                sprintf(token_val, "%c", value);
            }
            break;
        }
        case TOKEN_STRING_LITERAL: {
            char *str = token->string_value;
            token_name = strdup("literal(string)");
            token_val = strdup(str);
            break;
        }
        case TOKEN_DELIMITER: {
            uint32_t delim = token->delim;
            token_name = strdup("delim");
            switch (delim) {
                case DELIM_NEWLINE: {
                    token_val = strdup("nl");
                    break;
                }
                case DELIM_BRACE_OPEN: {
                    token_val = strdup("{");
                    break;
                }
                case DELIM_BRACE_CLOSE: {
                    token_val = strdup("}");
                    break;
                }
                case DELIM_BRACKET_OPEN: {
                    token_val = strdup("[");
                    break;
                }
                case DELIM_BRACKET_CLOSE: {
                    token_val = strdup("]");
                    break;
                }
                case DELIM_PARENTESIS_OPEN: {
                    token_val = strdup("(");
                    break;
                }
                case DELIM_PARENTESIS_CLOSE: {
                    token_val = strdup(")");
                    break;
                }
                case DELIM_DOT: {
                    token_val = strdup(".");
                    break;
                }
                case DELIM_COMMA: {
                    token_val = strdup(",");
                    break;
                }
                case DELIM_COLON: {
                    token_val = strdup(":");
                    break;
                }
                default: {
                    token_val = strdup("!unknown!");
                    break;
                }
            }
            break;
        }
        case TOKEN_EOF: {
            return strdup("<eof>");
        }
        default: {
            return strdup("<no-type>");
        }
    }

    char token_template[] = "<%s=%s %d:%d>";
    buffer = new char[strlen(token_template) + strlen(token_name) + strlen(token_val) + 256];
    sprintf(buffer, token_template, token_name, token_val, token->line, token->offset);
    delete token_name;
    delete token_val;

    return buffer;
}

int build_integer_literal(token_t *token, uint8_t is_hex) {
    int32_t current = accum_symbols_size - 1;
    uint32_t value = 0;
    uint32_t mul = 1;
    int32_t end = is_hex ? 1 : -1;
    uint32_t mul_mul = is_hex ? 16 : 10;
    // read until the x|X expected at position 1
    while (current > end) {
        symbol_t x = accum_buffer[current];
        uint32_t inc = HEX_TO_INT(x);
        value += inc * mul;
        mul *= mul_mul;
        --current;
    }
    bzero(accum_buffer, accum_symbols_size);
    accum_symbols_size = 0;
    token->type = TOKEN_INT_LITERAL;
    token->int_value = value;
    return 0;
}

int build_float_literal(token_t *token, uint8_t is_double) {
    size_t n_size = sizeof(symbol_t) * accum_symbols_size;
    char *float_value = new char[n_size + 1];
    bzero(float_value, n_size + 1);
    memcpy(float_value, accum_buffer, n_size);
    bzero(accum_buffer, n_size);
    accum_symbols_size = 0;
    token->type = TOKEN_FLOAT_LITERAL;
    token->float_value = float_value;
    return 0;
}

int build_string_literal(token_t *token, uint8_t has_trailing_quotes) {
    size_t n_size = sizeof(symbol_t) * (accum_symbols_size - (has_trailing_quotes ? 3 : 0));
    char *str_value = new char[n_size + 1];
    bzero(str_value, n_size + 1);
    memcpy(str_value, accum_buffer, n_size);
    bzero(accum_buffer, n_size);
    accum_symbols_size = 0;
    token->type = TOKEN_STRING_LITERAL;
    token->float_value = str_value;
    return 0;
}
