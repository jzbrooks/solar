#pragma once

#include <vector>
#include <unordered_map>

#include "ast.hpp"
#include "lexer.hpp"
#include "token.hpp"

class Parser;

typedef ast::Node* (Parser::*PrefixRule)();
typedef ast::Node* (Parser::*InfixRule)(ast::Node*);

enum class Precedence {
    LOWEST = 0,
    EQUALS,
    INEQUALITY,
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

    ast::Node* number();
    ast::Node* variable();
    ast::Node* binary(ast::Node*);
    ast::Node* conditional();
    ast::Node* expression(Precedence precedence);
    ast::Node* function();
    ast::Node* block();
    ast::Node* statement();

    void advance();
    void consume(Token::Kind kind, const char* message);
    void error(const char* message) const;
    void error(const Token& token, const char* message) const;

public:
    explicit Parser(Lexer* lexer);
    ast::Program* parseProgram();
};

