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

/// symbol type
typedef char symbol_t;

/**
 * token_t represents language lexeme
 * Content of the region should be read with accordance with the token type
 *
 */
typedef struct {
    uint8_t type;
    union {
        uint32_t keyword;
        uint32_t operator;
        bool_t bool_value;
        int32_t int_value;
        float float_value;
        char *string_value;
    };
    char *ident_value;
} token_t;

/**
 * On demand returns next token extracted from the input stream, char by char obtained via lex_next_symbol()
 * Requiring the next char of the input stream will return the next char after the last one of the token
 * Represents
 * @return
 */
token_t lex_next();

void on_lex_error(const char *error_desc);

//TODO: add other keywords and fix numbers
#define KEYWORD_IF      0x00000001U

///Arithmetic Operators
#define OP_ADD              0x00000001U //  +  // Addition
#define OP_SUB              0x00000002U //  -  // Subtraction
#define OP_MULT             0x00000003U //  *  // Multiplication
#define OP_DIV              0x00000004U //  /  // Division
#define OP_MOD              0x00000005U //  %  // Modulus
#define OP_EXP              0x00000006U // **  // Exponent
///Relational Operators
#define OP_EQ_TO            0x00000101U // ==  // Equal to
#define OP_NEQ_TO           0x00000102U // !=  // Not equal to
#define OP_GT_THAN          0x00000103U //  >  // Greater than
#define OP_LS_THAN          0x00000104U //  <  // Less than
#define OP_GT_THAN_EQ_TO    0x00000105U // >=  // Greater than equal to
#define OP_LS_THAN_EQ_TO    0x00000106U // <=  // Less than equal to
///Logical Operators
#define OP_L_AND            0x00000201U // &&  // Logical AND
#define OP_L_OR             0x00000202U // ||  // Logical OR
#define OP_L_NOT            0x00000203U //  !  // Logical NOT
///Assignment Operators
#define OP_ASSIGN           0x00000301U //  =  // Simple assignment
#define OP_ADD_ASSIGN       0x00000302U // +=  // Add and Assign
#define OP_SUB_ASSIGN       0x00000303U // -=  // Subtract and Assign
#define OP_MULT_ASSIGN      0x00000304U // *=  //
#define OP_DIV_ASSIGN       0x00000305U // /=  //
#define OP_MOD_ASSIGN       0x00000306U // %=  //
#define OP_EXP_ASSIGN       0x00000307U // **= //
#define OP_LSH_ASSIGN       0x00000308U // <<= // Left shift and Assign
#define OP_RSH_ASSIGN       0x00000309U // >>= // Right shift and Assign
#define OP_B_AND_ASSIGN     0x0000030aU // &=  // Bitwise AND and Assign
#define OP_B_OR_ASSIGN      0x0000030bU // |=  // Bitwise inclusive OR and Assign
#define OP_B_XOR_ASSIGN     0x0000030cU // ^=  // Bitwise exclusive OR and Assign
///Bitwise Operators
#define OP_B_AND            0x00000401U //  &  // Bitwise AND
#define OP_B_OR             0x00000402U //  |  // Bitwise OR
#define OP_B_XOR            0x00000403U //  ^  // Bitwise XOR
#define OP_LSH              0x00000404U // <<  // Bitwise left shift
#define OP_RSH              0x00000405U // >>  // Bitwise right shift
#define OP_COMPL            0x00000406U //  ~  // Bitwise ones complement
#define OP_RSH_Z            0x00000407U // >>> // Bitwise right shift zero fill

#endif //CC_LABS_LEXER_H
