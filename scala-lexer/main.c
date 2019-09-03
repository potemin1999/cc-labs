//
// Created by Ilya Potemin on 9/2/19.
//

#include <stdio.h>
#include "lexer.h"

void on_lex_error(const char *error_desc) {
    fprintf(stderr, "%s\n", error_desc);
}

int main(int argc, const char **argv) {
    //TODO: call lexer with test data
    if (argc > 1) {
        FILE *file;
        if (file = fopen(argv[1], "rb")) {
            lex_input(file);
            printf("Reading file %s\n", argv[1]);
        } else {
            printf("Unable to open file %s\n", argv[1]);
        }
    }
    token_t token;
    do {
        token = lex_next();
        char *tok_str = token_to_string(&token);
        printf("%s ", tok_str);
        free(tok_str);
    } while (token.type != TOKEN_EOF && token.type);
    return 0;
}