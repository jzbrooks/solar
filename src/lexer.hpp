#pragma once

#include "token.hpp"
#include <vector>

struct Lexer {
  std::vector<char> *input;
  size_t offset;
  size_t line;

  Token next();

private:
  void eat_whitespace();
  std::string extractLexeme(size_t length) const;
  Token read_word() const;
  Token read_number() const;
  Token read_string();
  bool match(char) const;
};
