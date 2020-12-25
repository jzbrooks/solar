#pragma once

#include <vector>
#include "token.hpp"

struct Lexer {
    std::vector<char>* input;
    int current;
    int line;

    Token next();

private:
    void eat_whitespace();
    Token read_word() const;
    Token read_number() const;
    bool match(char) const;
};

static bool isNumber(const char& c) {
    return isdigit(c) || c == '.' || c == 'i' || c == 'u' || c == 'f';
}