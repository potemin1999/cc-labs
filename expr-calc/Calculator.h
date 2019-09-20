/**
 * Created by ilya on 9/17/19.
 */

#ifndef CC_LABS_CALCULATOR_H
#define CC_LABS_CALCULATOR_H


#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

typedef size_t Size;
typedef char Symbol;
typedef int Value;

enum TokenType {
    TYPE_UNKNOWN = 0,
    OPERATOR,
    DELIMITER,
    VALUE,
};

const char *TokenTypeToString(TokenType type);

enum Operator {
    OPER_UNKNOWN = 0,
    GREATER_THAN,
    LESS_THAN,
    EQUAL_TO,
    ADDITION,
    SUBTRACTION,
    MULTIPLICATION,
    DIVISION,
};

const char *OperatorToString(Operator oper);

enum Delimiter {
    DELIM_UNKNOWN = 0,
    PAREN_OPEN,
    PAREN_CLOSE,
};

struct Token {
    TokenType type;
    union {
        Operator oper;
        Value value{};
        Delimiter delim;
    };

    constexpr Token() : type(TYPE_UNKNOWN) {
        oper = OPER_UNKNOWN;
        value = 0;
        delim = DELIM_UNKNOWN;
    }

private:

    constexpr explicit Token(Operator oper) : Token() {
        this->type = TokenType::OPERATOR;
        this->oper = oper;
    }

    constexpr explicit Token(Value value) : Token() {
        this->type = TokenType::VALUE;
        this->value = value;
    }

    constexpr explicit Token(Delimiter delim) : Token() {
        this->type = TokenType::DELIMITER;
        this->delim = delim;
    }

public:

    static constexpr Token makeOperatorToken(Operator oper) {
        return Token(oper);
    }

    static constexpr Token makeDelimiterToken(Delimiter delimiter) {
        return Token(delimiter);
    }

    static constexpr Token makeValueToken(Value value) {
        return Token(value);
    }

};

class Lexer {
private:

    const static Size stashBufferCap = 128;
    const static Size readBufferCap = 128;

    int stashBufferPtr;
    Symbol stashBuffer[stashBufferCap]{};

    int readBufferPtr;
    Size readBufferSize;
    Symbol readBuffer[readBufferCap]{};

    const char *source = nullptr;

public:

    constexpr Lexer() :
            stashBufferPtr(0),
            readBufferPtr(0),
            readBufferSize(readBufferCap) {}

    constexpr explicit Lexer(const char *source) :
            Lexer() {
        this->source = source;
    }

    Token nextToken();


private:

    static inline bool isDigit(Symbol symbol) {
        return symbol >= '0' && symbol <= '9';
    }

    int stashSymbol(Symbol symbol) {
        stashBuffer[stashBufferPtr] = symbol;
        return ++stashBufferPtr;
    }

    Symbol nextSymbol();


};

enum ExpressionType {
    EXPRESSION,
    RELATION,
    TERM,
    FACTOR,
    PRIMARY,
    INTEGER,
};

struct Expression {
    ExpressionType type;

    explicit Expression(ExpressionType type) : type(type) {}
};

struct Integer : Expression {
    Value value;

    explicit Integer(Value value) :
            Expression(INTEGER),
            value(value) {}
};

struct Primary : Expression {
    Integer *integer;
    Expression *expression;

    explicit Primary(Integer *integer) :
            Expression(PRIMARY),
            integer(integer), expression(nullptr) {}

    explicit Primary(Expression *expression) :
            Expression(PRIMARY),
            integer(nullptr), expression(expression) {}
};

struct Factor : Expression {
    Primary *operand;
    Operator oper{};
    Factor *next{};

    explicit Factor(Primary *left) :
            Expression(FACTOR),
            operand(left) {}

    explicit Factor(Primary *left, Operator oper, Factor *prev) :
            Expression(FACTOR),
            operand(left), oper(oper), next(nullptr) { prev->next = this; }
};

struct Term : Expression {
    Factor *operand;
    Operator oper{};
    Term *next{};

    explicit Term(Factor *left) :
            Expression(TERM),
            operand(left) {}

    explicit Term(Factor *left, Operator oper, Term *prev) :
            Expression(TERM),
            operand(left), oper(oper), next(nullptr) { prev->next = this; }
};

struct Relation : Expression {
    Term *left;
    Operator oper{};
    Term *right{};

    explicit Relation(Term *left) :
            Expression(RELATION),
            left(left) {}

    explicit Relation(Term *left, Operator oper, Term *right) :
            Expression(RELATION),
            left(left), oper(oper), right(right) {}
};


class Parser {
private:
    Lexer &lexer;
    Token token;
    bool needReadToken = true;

public:
    constexpr explicit Parser(Lexer &lexer) : lexer(lexer) {}

    Expression *parse() {
        return parseExpression();
    }

private:

    Token &peekToken() {
        if (needReadToken) {
            token = lexer.nextToken();
            needReadToken = false;
        }
        return token;
    }


    static inline void reportError(const char *msg) {
        std::printf("parsing error : %s", msg);
    }

    void commitToken() {
        needReadToken = true;
    }

    Expression *parseExpression();

    Relation *parseRelation();

    Term *parseTerm();

    Factor *parseFactor();

    Primary *parsePrimary();

    Integer *parseInteger();

};


class Calculator {

public:
    Calculator() = default;

    static Value calculate(Expression *expression);

private:

    static Value applyOperator(Value v1, Operator oper, Value v2);

    static inline void reportError(const char *msg) {
        printf("calculation error : %s", msg);
    }

};

#endif //CC_LABS_CALCULATOR_H
