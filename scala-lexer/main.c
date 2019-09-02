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
    return 0;
}