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
        return { Token::Kind::END, "" };
    }

    switch (ch) {
        case '+':
            token = { Token::Kind::PLUS, extractLexeme(1) };
            break;
        case '-':
            if (match('>')) {
                token = { Token::Kind::ARROW, extractLexeme(2) };
            } else {
                token = { Token::Kind::MINUS, extractLexeme(1) };
            }
            break;
        case '*':
            token = { Token::Kind::STAR, extractLexeme(1) };
            break;
        case '/':
            token = { Token::Kind::SLASH, extractLexeme(1) };
            break;
        case '=':
            if (match('=')) {
                token = { Token::Kind::EQUAL, extractLexeme(2) };
            } else {
                token = { Token::Kind::ASSIGN, extractLexeme(1) };
            }
            break;
        case '<':
            if (match('=')) {
                token = { Token::Kind::LESS_EQUAL, extractLexeme(2) };
            } else {
                token = { Token::Kind::LESS, extractLexeme(1) };
            }
            break;
        case '>':
            if (match('=')) {
                token = { Token::Kind::GREATER_EQUAL, extractLexeme(2) };
            } else {
                token = { Token::Kind::GREATER, extractLexeme(1) };
            }
            break;
        case '(':
            token = { Token::Kind::LPAREN, extractLexeme(1) };
            break;
        case ')':
            token = { Token::Kind::RPAREN, extractLexeme(1) };
            break;
        case '{':
            token = { Token::Kind::LBRACE, extractLexeme(1) };
            break;
        case '}':
            token = { Token::Kind::RBRACE, extractLexeme(1) };
            break;
        case '!':
            if (match('=')) {
                token = { Token::Kind::NOT_EQUAL, extractLexeme(2) };
            } else {
                token = { Token::Kind::NEGATE, extractLexeme(1) };
            }
            break;
        default:
            if (isalpha(ch) || ch == '_') {
                token = read_word();
            } else if (isdigit(ch)) {
                token = read_number();
            } else {
                token = { Token::Kind::INVALID, extractLexeme(1) };
            }
            break;
    }

    current += token.lexeme.size();

    return token;
}

std::string Lexer::extractLexeme(size_t length) const {
    return { input->data() + current, length };
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
        return Token { reserved_word->second, extractLexeme(length) };
    }

    return Token { Token::Kind::IDENTIFIER, extractLexeme(length) };
}

Token Lexer::read_number() const {
    auto length = 0;
    for (auto it = input->begin() + current; it != input->end() && (isdigit(*it) || *it == '.'); ++it) {
        length += 1;
    }

    if (input->size() > current + length + 3) {
        std::string slice(input->data() + current + length, 3);
        if (slice == "u32" || slice == "u64" || slice == "f32" || slice == "i32") {
            length += 3;
        }
    }

    return Token { Token::Kind::NUMBER, extractLexeme(length) };
}

bool Lexer::match(char character) const {
    int next = current + 1;
    return next < input->size() && (*input)[next] == character;
}