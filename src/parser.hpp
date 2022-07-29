#pragma once

#include <unordered_map>
#include <vector>

#include "ast.hpp"
#include "lexer.hpp"
#include "token.hpp"

class Parser;

typedef ast::Node *(Parser::*PrefixRule)();
typedef ast::Node *(Parser::*InfixRule)(ast::Node *);

enum class Precedence {
  NONE = 0,
  ASSIGNMENT,
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
  Lexer *lexer;
  Token current{};
  Token previous{};
  std::vector<std::string> errors;
  std::unordered_map<Token::Kind, ParseRule> rules;

  ast::Node *number();
  ast::Node *variable();
  ast::Node *binary(ast::Node *);
  ast::Node *conditional();
  ast::Node *expression(Precedence precedence);
  ast::Node *function();
  ast::Node *block();
  ast::Node *statement();
  ast::Node *ret();
  ast::Node *call(ast::Node *);
  ast::Node *str();
  ast::Node *assignment();
  ast::Node *grouping();

  ast::TypeInfo type();

  void advance();
  void consume(Token::Kind kind, const std::string &message);
  void error(const std::string &message);
  void error(const Token &token, const std::string &message);

public:
  explicit Parser(Lexer *lexer);
  ast::Program *parse_program();
};
