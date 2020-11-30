#include "catch.hpp"

#include "../src/lexer.hpp"

TEST_CASE( "Identifiers are given the appropriate kind", "[lexer]" ) {
    std::vector<char> input { 'x' };
    Lexer lexer{&input, 0};

    REQUIRE( lexer.next().kind == Token::Kind::IDENTIFIER );
}

TEST_CASE( "Plus is given the appropriate kind", "[lexer]" ) {
    std::vector<char> input { '+' };
    Lexer lexer{&input, 0};

    REQUIRE( lexer.next().kind == Token::Kind::PLUS );
}

TEST_CASE( "Leading tabs are eaten", "[lexer]" ) {
    std::vector<char> input { '\t', '+' };
    Lexer lexer{&input, 0};

    REQUIRE( lexer.next().kind == Token::Kind::PLUS );
}

TEST_CASE( "Leading spaces are eaten", "[lexer]" ) {
    std::vector<char> input { ' ', ' ', '+' };
    Lexer lexer{&input, 0};

    REQUIRE( lexer.next().kind == Token::Kind::PLUS );
}

TEST_CASE( "Leading newlines are eaten", "[lexer]" ) {
    std::vector<char> input { '\n', '+' };
    Lexer lexer{&input, 0};

    REQUIRE( lexer.next().kind == Token::Kind::PLUS );
}

TEST_CASE( "Multi-letter identifier are lexed", "[lexer]" ) {
    std::vector<char> input { 't', 'e', 's', 't' };
    Lexer lexer{&input, 0};

    auto token = lexer.next();

    REQUIRE( token.kind == Token::Kind::IDENTIFIER );
    REQUIRE( token.length == 4 );
}

TEST_CASE( "Identifier can begin with '_'", "[lexer]" ) {
    std::vector<char> input { '_', 't', 'e', 's', 't' };
    Lexer lexer{&input, 0};

    auto token = lexer.next();

    REQUIRE( token.kind == Token::Kind::IDENTIFIER );
    REQUIRE( token.length == 5 );
}

TEST_CASE( "Reserved words are lexed", "[lexer]" ) {
    std::vector<char> input { 'f', 'u', 'n', 'c' };
    Lexer lexer{&input, 0};

    auto token = lexer.next();

    REQUIRE( token.kind == Token::Kind::FUNC );
    REQUIRE( token.length == 4 );
}

TEST_CASE( "Numbers words are lexed", "[lexer]" ) {
    std::vector<char> input { '9', '3', '2', '1' };
    Lexer lexer{&input, 0};

    auto token = lexer.next();

    REQUIRE( token.kind == Token::Kind::NUMBER );
    REQUIRE(token.length == 4 );
}

TEST_CASE( "Arrows are lexed", "[lexer]" ) {
    std::vector<char> input { '-', '>' };
    Lexer lexer{&input, 0};

    auto token = lexer.next();

    REQUIRE( token.kind == Token::Kind::ARROW );
    REQUIRE( token.length == 2 );
}

TEST_CASE( "Less equal comparisons are lexed", "[lexer]" ) {
    std::vector<char> input { '<', '=' };
    Lexer lexer{&input, 0};

    auto token = lexer.next();

    REQUIRE( token.kind == Token::Kind::LESS_EQUAL );
    REQUIRE( token.length == 2 );
}

TEST_CASE( "Less comparisons are lexed", "[lexer]" ) {
    std::vector<char> input { '<' };
    Lexer lexer{&input, 0};

    auto token = lexer.next();

    REQUIRE( token.kind == Token::Kind::LESS );
    REQUIRE( token.length == 1 );
}

TEST_CASE( "Multiple tokens are lexed", "[lexer]" ) {
    std::vector<char> input { '5', '>', '=', '5' };
    Lexer lexer{&input, 0};

    auto first = lexer.next();

    REQUIRE( first.kind == Token::Kind::NUMBER );
    REQUIRE( first.start == 0 );
    REQUIRE( first.length == 1 );

    auto second = lexer.next();

    REQUIRE( second.kind == Token::Kind::GREATER_EQUAL );
    REQUIRE( second.start == 1 );
    REQUIRE( second.length == 2 );

    auto third = lexer.next();

    REQUIRE( third.kind == Token::Kind::NUMBER );
    REQUIRE( third.start == 3 );
    REQUIRE( third.length == 1 );
}
