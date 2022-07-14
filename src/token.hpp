#pragma once

#include <string>
#include <utility>

struct Token {
  enum class Kind : int {
    IDENTIFIER = 0,

    NUMBER,

    STRING,

    FUNC,
    IF,
    ELSE,
    VAR,

    PLUS,
    MINUS,
    STAR,
    SLASH,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    COMMA,
    COLON,

    ARROW,

    LESS,
    GREATER,
    LESS_EQUAL,
    GREATER_EQUAL,
    EQUAL,
    NOT_EQUAL,

    NEGATE,

    ASSIGN,

    RETURN,

    INVALID,

    END,
  };

  Kind kind;
  std::string lexeme;
  int line;

  Token() = default;
  Token(Kind kind, std::string lexeme)
      : kind(kind), lexeme(std::move(lexeme)) {}
  Token(const Token &other)
      : kind(other.kind), lexeme(other.lexeme), line(other.line){};

  bool operator==(const Token &other) const {
    return kind == other.kind && lexeme == other.lexeme;
  };
};

static auto name(Token::Kind kind) -> const char * {
  switch (kind) {
  case Token::Kind::IDENTIFIER:
    return "IDENTIFIER\0";
  case Token::Kind::FUNC:
    return "FUNC\0";
  case Token::Kind::IF:
    return "IF\0";
  case Token::Kind::ELSE:
    return "ELSE\0";
  case Token::Kind::PLUS:
    return "ADD\0";
  case Token::Kind::MINUS:
    return "MINUS\0";
  case Token::Kind::STAR:
    return "STAR\0";
  case Token::Kind::SLASH:
    return "SLASH\0";
  case Token::Kind::LPAREN:
    return "LPAREN\0";
  case Token::Kind::RPAREN:
    return "RPAREN\0";
  case Token::Kind::LBRACE:
    return "LBRACE\0";
  case Token::Kind::RBRACE:
    return "RBRACE\0";
  case Token::Kind::ARROW:
    return "ARROW\0";
  case Token::Kind::LESS:
    return "LESS\0";
  case Token::Kind::GREATER:
    return "GREATER\0";
  case Token::Kind::LESS_EQUAL:
    return "LESS_EQUAL\0";
  case Token::Kind::GREATER_EQUAL:
    return "GREATER_EQUAL\0";
  case Token::Kind::EQUAL:
    return "EQUAL\0";
  case Token::Kind::NOT_EQUAL:
    return "NOT_EQUAL\0";
  case Token::Kind::RETURN:
    return "RETURN\0";
  case Token::Kind::END:
    return "END\0";
  default:
    return "UNKNOWN\0";
  }
}
