#include "catch.hpp"

#include "../src/parser.hpp"

TEST_CASE( "Parse integer expression. ", "[parser]") {
    std::vector<char> input { '1', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(int32<1>)");
}

TEST_CASE( "Parse integer addition expression. ", "[parser]") {
    std::vector<char> input { '1', '+', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(+ (int32<1>) (int32<2>))");
}

TEST_CASE( "Parse integer subtraction expression. ", "[parser]") {
    std::vector<char> input { '1', '-', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(- (int32<1>) (int32<2>))");
}

TEST_CASE( "Parse integer multiplication expression. ", "[parser]") {
    std::vector<char> input { '1', '*', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(* (int32<1>) (int32<2>))");
}

TEST_CASE( "Parse integer division expression. ", "[parser]") {
    std::vector<char> input { '1', '/', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(/ (int32<1>) (int32<2>))");
}

TEST_CASE( "Parse multiple integer addition expression. ", "[parser]") {
    std::vector<char> input { '1', '+', '2', '+', '3', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);

    auto program = parser.parseProgram();

    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(+ (+ (int32<1>) (int32<2>)) (int32<3>))");
}

TEST_CASE( "Parse term/factor precedence expression. ", "[parser]") {
    std::vector<char> input { '1', '+', '2', '/', '3', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);

    auto program = parser.parseProgram();

    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(+ (int32<1>) (/ (int32<2>) (int32<3>)))");
}
