#include "catch.hpp"

#include "../src/parser.hpp"

TEST_CASE( "Parse integer expression. ", "[parser]") {
    std::vector<char> input { '1', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(int64<1>)");
}

TEST_CASE( "Parse float expression. ", "[parser]") {
    std::vector<char> input { '1', '.', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(float64<1>)");
}

TEST_CASE( "Parse literal qualifier expression. ", "[parser]") {
    std::vector<char> input { '1', 'u', '3', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(uint32<1>)");
}

TEST_CASE( "Parse integer addition expression. ", "[parser]") {
    std::vector<char> input { '1', '+', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(+ (int64<1>) (int64<2>))");
}

TEST_CASE( "Parse integer subtraction expression. ", "[parser]") {
    std::vector<char> input { '1', '-', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(- (int64<1>) (int64<2>))");
}

TEST_CASE( "Parse integer multiplication expression. ", "[parser]") {
    std::vector<char> input { '1', '*', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(* (int64<1>) (int64<2>))");
}

TEST_CASE( "Parse integer division expression. ", "[parser]") {
    std::vector<char> input { '1', '/', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(/ (int64<1>) (int64<2>))");
}

TEST_CASE( "Parse multiple integer addition expression. ", "[parser]") {
    std::vector<char> input { '1', '+', '2', '+', '3', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);

    auto program = parser.parseProgram();

    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(+ (+ (int64<1>) (int64<2>)) (int64<3>))");
}

TEST_CASE( "Parse term/factor precedence expression. ", "[parser]") {
    std::vector<char> input { '1', '+', '2', '/', '3', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);

    auto program = parser.parseProgram();

    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(+ (int64<1>) (/ (int64<2>) (int64<3>)))");
}
