/**
 * Created by ilya on 9/16/19.
 */

#include "Calculator.h"

const char *TokenTypeToString(TokenType type) {
    switch (type) {
        case OPERATOR: return "OPERATOR";
        case DELIMITER: return "DELIMITER";
        case VALUE: return "VALUE";
        default: return "TYPE_UNKNOWN";
    }
}

const char *OperatorToString(Operator oper) {
    switch (oper) {
        case GREATER_THAN: return "GREATER_THAN";
        case LESS_THAN: return "LESS_THAN";
        case EQUAL_TO: return "EQUAL_TO";
        case ADDITION: return "ADDITION";
        case SUBTRACTION: return "SUBTRACTION";
        case MULTIPLICATION: return "MULTIPLICATION";
        case DIVISION: return "DIVISION";
        default: return "OPER_UNKNOWN";
    }
}

Symbol Lexer::nextSymbol() {
    if (source != nullptr) {
        if (source[readBufferPtr] == '\0') {
            return '\0';
        } else {
            return source[readBufferPtr];
        }
    }
    if (readBufferPtr >= readBufferSize) {
        readBufferSize = std::fread(readBuffer, 1, readBufferCap, stdin);
        if (readBufferSize == 0) {
            return '\0';
        }
        readBufferPtr = 0;
    }
    return readBuffer[readBufferPtr];
}

Token Lexer::nextToken() {
    Symbol s1 = nextSymbol();
    readBufferPtr++;
    switch (s1) {
        case '>': return Token::makeOperatorToken(Operator::GREATER_THAN);
        case '<': return Token::makeOperatorToken(Operator::LESS_THAN);
        case '=': return Token::makeOperatorToken(Operator::EQUAL_TO);
        case '+': return Token::makeOperatorToken(Operator::ADDITION);
        case '-': return Token::makeOperatorToken(Operator::SUBTRACTION);
        case '*': return Token::makeOperatorToken(Operator::MULTIPLICATION);
        case '/': return Token::makeOperatorToken(Operator::DIVISION);
        case '(': return Token::makeDelimiterToken(Delimiter::PAREN_OPEN);
        case ')': return Token::makeDelimiterToken(Delimiter::PAREN_CLOSE);
        default: {
            --readBufferPtr;
            while (s1 == ' ') {
                std::fprintf(stderr, "WARN: spaces are not stated in the grammar, skipping them by default\n");
                ++readBufferPtr;
                s1 = nextSymbol();
            }
            while (isDigit(s1)) {
                stashSymbol(s1);
                ++readBufferPtr;
                s1 = nextSymbol();
            }
            char *endPtr = nullptr;
            long value = strtol(stashBuffer, &endPtr, 10);
            std::memset(stashBuffer, 0, stashBufferPtr);
            stashBufferPtr = 0;
            return Token::makeValueToken(static_cast<Value>(value));
        }
    }
}

Expression *Parser::parseExpression() {
    return parseRelation();
}

Relation *Parser::parseRelation() {
    Term *left = parseTerm();
    if (peekToken().type == OPERATOR) {
        Operator oper = peekToken().oper;
        if (oper != Operator::EQUAL_TO &&
            oper != Operator::LESS_THAN &&
            oper != Operator::GREATER_THAN) {
            return new Relation(left);
        }
        commitToken();
        Term *right = parseTerm();
        return new Relation(left, oper, right);
    } else {
        return new Relation(left);
    }
}

Term *Parser::parseTerm() {
    Factor *left = parseFactor();
    auto first = new Term(left);
    auto last = first;

    while (peekToken().type == OPERATOR) {
        Operator oper = peekToken().oper;
        if (oper != Operator::ADDITION &&
            oper != Operator::SUBTRACTION) {
            return first;
        }
        commitToken();
        Factor *right = parseFactor();
        last = new Term(right, oper, last);
    }
    return first;
}

Factor *Parser::parseFactor() {
    Primary *left = parsePrimary();
    auto first = new Factor(left);
    auto last = first;

    while (peekToken().type == OPERATOR) {
        Operator oper = peekToken().oper;
        if (oper != Operator::MULTIPLICATION &&
            oper != Operator::DIVISION) {
            return first;
        }
        commitToken();
        Primary *right = parsePrimary();
        last = new Factor(right, oper, last);
    }
    return first;
}

Primary *Parser::parsePrimary() {
    if (peekToken().type == DELIMITER && peekToken().delim == Delimiter::PAREN_OPEN) {
        commitToken();
        Expression *expr = parseExpression();
        commitToken();
        return new Primary(expr);
    } else {
        Integer *integer = parseInteger();
        return new Primary(integer);
    }
}

Integer *Parser::parseInteger() {
    if (peekToken().type != VALUE) {
        char *buffer = new char[256];
        sprintf(buffer, "expected VALUE type, got %s", TokenTypeToString(peekToken().type));
        Parser::reportError(buffer);
        delete[] buffer;
    }
    Value value = peekToken().value;
    commitToken();
    return new Integer(value);
}

Value Calculator::applyOperator(Value v1, Operator oper, Value v2) {
    printf(" applying %s on %d, %d\n", OperatorToString(oper), v1, v2);
    switch (oper) {
        case Operator::GREATER_THAN: {
            return v1 > v2 ? 1 : 0;
        }
        case Operator::LESS_THAN: {
            return v1 < v2 ? 1 : 0;
        }
        case Operator::EQUAL_TO: {
            return v1 == v2 ? 1 : 0;
        }
        case Operator::ADDITION: {
            return v1 + v2;
        }
        case Operator::SUBTRACTION: {
            return v1 - v2;
        }
        case Operator::MULTIPLICATION: {
            return v1 * v2;
        }
        case Operator::DIVISION: {
            if (v2 == 0) {
                Calculator::reportError("VALUE should not be divided by zero");
            }
            return v1 / v2;
        }
        default : return -1;
    }
}


Value Calculator::calculate(Expression *expression) {
    if (expression == nullptr) {
        Calculator::reportError("Unable to calculate nullptr Expression, program logic error");
        std::exit(-1);
    }
    switch (expression->type) {
        case ExpressionType::EXPRESSION:
        case ExpressionType::RELATION: {
            auto relation = reinterpret_cast<Relation *>(expression);
            Value lValue = calculate(relation->left);
            if (relation->right) {
                Value rValue = calculate(relation->right);
                return applyOperator(lValue, relation->oper, rValue);
            } else {
                return lValue;
            }
        }
        case ExpressionType::TERM: {
            auto term = reinterpret_cast<Term *> (expression);
            Value lValue = calculate(term->operand);
            while (term->next) {
                term = term->next;
                Value rValue = calculate(term->operand);
                lValue = applyOperator(lValue, term->oper, rValue);
            }
            return lValue;
        }
        case ExpressionType::FACTOR: {
            auto factor = reinterpret_cast<Factor *> (expression);
            Value lValue = calculate(factor->operand);
            while (factor->next) {
                factor = factor->next;
                Value rValue = calculate(factor->operand);
                lValue = applyOperator(lValue, factor->oper, rValue);
            }
            return lValue;
        }
        case ExpressionType::PRIMARY: {
            auto primary = reinterpret_cast<Primary *> (expression);
            if (primary->integer) {
                return calculate(primary->integer);
            } else {
                return calculate(primary->expression);
            }
        }
        case ExpressionType::INTEGER : {
            auto integer = reinterpret_cast<Integer *>(expression);
            return integer->value;
        }
    }
    return 0;
}
