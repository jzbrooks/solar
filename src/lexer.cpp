#include "lexer.hpp"

#include <cctype>
#include <unordered_map>
#include <sstream>

static std::unordered_map<std::string, Token::Kind> reserved_words {
    {"if", Token::Kind::IF},
    {"else", Token::Kind::ELSE},
    {"func", Token::Kind::FUNC},
};

Token Lexer::next() {
    Token token; // NOLINT(cppcoreguidelines-pro-type-member-init)

    eat_whitespace();

    token.line = line;

    auto ch = input->at(current);

    if (current >= input->size() || ch == EOF) {
        return { Token::Kind::END };
    }

    switch (ch) {
        case '+':
            token = { Token::Kind::PLUS, current, 1 };
            break;
        case '-':
            if (match('>')) {
                token = { Token::Kind::ARROW, current, 2 };
            } else {
                token = { Token::Kind::MINUS, current,  1 };
            }
            break;
        case '*':
            token = { Token::Kind::STAR, current, 1 };
            break;
        case '/':
            token = { Token::Kind::SLASH, current, 1 };
            break;
        case '<':
            if (match('=')) {
                token = { Token::Kind::LESS_EQUAL, current, 2 };
            } else {
                token = { Token::Kind::LESS, current, 1 };
            }
            break;
        case '>':
            if (match('=')) {
                token = { Token::Kind::GREATER_EQUAL, current, 2 };
            } else {
                token = { Token::Kind::GREATER, current, 1 };
            }
            break;
        case '(':
            token = { Token::Kind::LPAREN, current, 1 };
            break;
        case ')':
            token = { Token::Kind::RPAREN, current, 1 };
            break;
        case '{':
            token = { Token::Kind::LBRACE, current, 1 };
            break;
        case '}':
            token = { Token::Kind::RBRACE, current, 1 };
            break;
        default:
            if (isalpha(ch) || ch == '_') {
                token = read_word();
            } else if (isdigit(ch)) {
                token = read_number();
            } else {
                token = { Token::Kind::INVALID, current, 1 };
            }
            break;
    }

    current += token.length;

    return token;
}

void Lexer::eat_whitespace() {
    for (auto it = input->begin() + current; it != input->end() && isspace(*it); ++it) {
        if (*it == '\n') line += 1;
        current += 1;
    }
}

Token Lexer::read_word() const {
    std::stringstream word_stream;

    for (auto it = input->begin() + current; it != input->end() && (isalnum(*it) || *it == '_'); ++it) {
        word_stream << *it;
    }

    auto word = word_stream.str();
    auto length = static_cast<int>(word.length());

    auto reserved_word = reserved_words.find(word);
    if (reserved_word != reserved_words.end()) {
        return Token { reserved_word->second, current, length };
    }

    return Token { Token::Kind::IDENTIFIER, current, length };
}

Token Lexer::read_number() const {
    auto length = 0;
    for (auto it = input->begin() + current; it != input->end() && isNumber(*it); ++it) {
        length += 1;
    }

    return Token { Token::Kind::NUMBER, current, length };
}

bool Lexer::match(char character) const {
    int next = current + 1;
    return next < input->size() && (*input)[next] == character;
}