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

TEST_CASE( "Parse comparison without else. ", "[parser]") {
     std::vector<char> input { '1', '=', '=', '3', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);

    auto program = parser.parseProgram();

    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(== (int64<1>) (int64<3>))");
}

TEST_CASE( "Parse condition expression. ", "[parser]") {
    std::vector<char> input { 'i', 'f', ' ', '1', '<', '3', '{', '3', '}', 'e', 'l', 's', 'e', '{', '0', '}', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);

    auto program = parser.parseProgram();

    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(if (< (int64<1>) (int64<3>)) then (int64<3>) otherwise (int64<0>))");
}

TEST_CASE( "Parse condition without else. ", "[parser]") {
    std::vector<char> input { 'i', 'f', ' ', '1', '!', '=', '3', '{', '3', '}', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);

    auto program = parser.parseProgram();

    auto statement = (ast::ExpressionStatement*)program->statements.front();
    REQUIRE(statement->expression->describe() == "(if (!= (int64<1>) (int64<3>)) then (int64<3>))");
}

TEST_CASE( "Parse functions. ", "[parser]") {
    std::vector<char> input { 'f', 'u', 'n', 'c', ' ', 'a', 'd', 'd', '(', ')', '{', '1', '+', '2', '}', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);

    auto program = parser.parseProgram();

    auto statement = (ast::Function*)program->statements.front();
    REQUIRE(statement->describe() == "(fn-def (fn-type add()  (block \n(+ (int64<1>) (int64<2>))\n))");
}

TEST_CASE( "Parse variable declarations. ", "[parser]") {
    std::vector<char> input { 'c', 'o', 'u', 'n', 't', '=', '0', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);

    auto program = parser.parseProgram();

    auto statement = program->statements.front();
    REQUIRE(statement->describe() == "(var-decl bool<count> (int64<0>))"); // this type should be corrected by inference
}
