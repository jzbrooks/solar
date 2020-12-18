#include "catch.hpp"

#include "../src/parser.hpp"

TEST_CASE( "Parse integer expression. ", "[parser]") {
    std::vector<char> input { '1' };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto expression = parser.parseProgram();

    REQUIRE( ((ast::IntExpression*)expression)->value.int32 == 1 );
}