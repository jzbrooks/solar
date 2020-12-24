#include "catch.hpp"

#include "../src/parser.hpp"

TEST_CASE( "Parse integer expression. ", "[parser]") {
    std::vector<char> input { '1', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();

    REQUIRE(((ast::IntExpression*)statement->expression)->value.int32 == 1 );
}

TEST_CASE( "Parse integer addition expression. ", "[parser]") {
    std::vector<char> input { '1', '+', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    auto leftExpr = ((ast::Binop*)statement->expression)->left;
    auto left = (ast::IntExpression*)leftExpr;
    auto rightExpr = ((ast::Binop*)statement->expression)->right;
    auto right = (ast::IntExpression*)rightExpr;

    REQUIRE(left->value.int32 == 1 );
    REQUIRE(right->value.int32 == 2 );
    REQUIRE(((ast::Operation)((ast::Binop*)statement->expression)->operation) == ast::Operation::ADD );
}

TEST_CASE( "Parse integer subtraction expression. ", "[parser]") {
    std::vector<char> input { '1', '-', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    auto leftExpr = ((ast::Binop*)statement->expression)->left;
    auto left = (ast::IntExpression*)leftExpr;
    auto rightExpr = ((ast::Binop*)statement->expression)->right;
    auto right = (ast::IntExpression*)rightExpr;

    REQUIRE(left->value.int32 == 1 );
    REQUIRE(right->value.int32 == 2 );
    REQUIRE(((ast::Operation)((ast::Binop*)statement->expression)->operation) == ast::Operation::SUBTRACT );
}

TEST_CASE( "Parse integer multiplication expression. ", "[parser]") {
    std::vector<char> input { '1', '*', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    auto leftExpr = ((ast::Binop*)statement->expression)->left;
    auto left = (ast::IntExpression*)leftExpr;
    auto rightExpr = ((ast::Binop*)statement->expression)->right;
    auto right = (ast::IntExpression*)rightExpr;

    REQUIRE(left->value.int32 == 1 );
    REQUIRE(right->value.int32 == 2 );
    REQUIRE(((ast::Operation)((ast::Binop*)statement->expression)->operation) == ast::Operation::MULTIPLY );
}

TEST_CASE( "Parse integer division expression. ", "[parser]") {
    std::vector<char> input { '1', '/', '2', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);
    auto program = parser.parseProgram();
    auto statement = (ast::ExpressionStatement*)program->statements.front();
    auto leftExpr = ((ast::Binop*)statement->expression)->left;
    auto left = (ast::IntExpression*)leftExpr;
    auto rightExpr = ((ast::Binop*)statement->expression)->right;
    auto right = (ast::IntExpression*)rightExpr;

    REQUIRE(left->value.int32 == 1 );
    REQUIRE(right->value.int32 == 2 );
    REQUIRE(((ast::Operation)((ast::Binop*)statement->expression)->operation) == ast::Operation::DIVIDE );
}

TEST_CASE( "Parse multiple integer addition expression. ", "[parser]") {
    std::vector<char> input { '1', '+', '2', '+', '3', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);

    auto program = parser.parseProgram();

    auto statement = (ast::ExpressionStatement*)program->statements.front();
    auto leftExpr = ((ast::Binop*)statement->expression)->left;
    auto left = (ast::Binop*)leftExpr;
    auto leftLeft = (ast::IntExpression*)left->left;
    auto leftRight = (ast::IntExpression*)left->right;
    auto rightExpr = ((ast::Binop*)statement->expression)->right;
    auto right = (ast::IntExpression*)rightExpr;

    REQUIRE(leftLeft->value.int32 == 1 );
    REQUIRE(((ast::Operation)left->operation) == ast::Operation::ADD);
    REQUIRE(leftRight->value.int32 == 2 );

    REQUIRE(((ast::Operation)((ast::Binop*)statement->expression)->operation) == ast::Operation::ADD);
    REQUIRE(right->value.int32 == 3 );
}

TEST_CASE( "Parse term/factor precedence expression. ", "[parser]") {
    std::vector<char> input { '1', '+', '2', '/', '3', EOF };
    Lexer lexer{&input, 0};
    Parser parser(&lexer);

    auto program = parser.parseProgram();

    auto statement = (ast::ExpressionStatement*)program->statements.front();
    auto leftExpr = ((ast::Binop*)statement->expression)->left;
    auto left = (ast::IntExpression*)leftExpr;

    auto rightExpr = ((ast::Binop*)statement->expression)->right;
    auto right = (ast::Binop*)rightExpr;
    auto rightLeft = (ast::IntExpression*)right->left;
    auto rightRight = (ast::IntExpression*)right->right;

    REQUIRE(left->value.int32 == 1 );
    REQUIRE(((ast::Operation)((ast::Binop*)statement->expression)->operation) == ast::Operation::ADD);

    REQUIRE(rightLeft->value.int32 == 2 );
    REQUIRE(((ast::Operation)right->operation) == ast::Operation::DIVIDE);
    REQUIRE(rightRight->value.int32 == 3 );
}
