#pragma once

#include <vector>
#include "token.hpp"

struct Lexer {
    std::vector<char>* input;
    int current;

    Token next();

private:
    void eat_whitespace();
    Token read_word() const;
    Token read_number() const;
    bool match(char) const;
};
