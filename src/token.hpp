#pragma once

struct Token {
    enum class Kind {
        IDENTIFIER,

        NUMBER,

        FUNC,
        IF,
        ELSE,
        
        PLUS,
        MINUS,
        STAR,
        SLASH,

        LPAREN,
        RPAREN,
        LBRACE,
        RBRACE,

        ARROW,

        LESS,
        GREATER,
        LESS_EQUAL,
        GREATER_EQUAL,
        EQUAL,
        NOT_EQUAL,

        RETURN,

        INVALID,

        END,
    };

    Kind kind;
    int start;
    int length;
};

auto name(Token::Kind kind) -> const char*;
