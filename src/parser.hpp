#pragma once

#include <vector>
#include <unordered_map>

#include "ast.hpp"
#include "lexer.hpp"
#include "token.hpp"

class Parser;

typedef ast::Expression* (Parser::*PrefixRule)();
typedef ast::Expression* (Parser::*InfixRule)(ast::Expression*);

enum class Precedence {
    LOWEST = 0,
    TERM,
    FACTOR,
    CALL,
};

struct ParseRule {
    PrefixRule prefix;
    InfixRule infix;
    Precedence precedence;
};

class Parser {
    Lexer* lexer;
    Token lookahead{};
    Token current{};
    std::unordered_map<Token::Kind, ParseRule> rules;

    ast::Expression* number();
    ast::Expression* variable();
    ast::Expression* binary(ast::Expression*);
    ast::Expression* conditional();
    ast::Expression* expression(Precedence precedence);

    void advance();
    void consume(Token::Kind kind, const char* message);
    void error(const char* message) const;
    void error(const Token& token, const char* message) const;

public:
    explicit Parser(Lexer* lexer);
    ast::Program* parseProgram();
};

