#include "catch/catch.hpp"

#include "../src/parser.hpp"

TEST_CASE("Parse integer expression. ", "[parser]") {
  std::vector<char> input{'1', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);
  auto program = parser.parse_program();
  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() == "(i64<1>)");
}

TEST_CASE("Parse float expression. ", "[parser]") {
  std::vector<char> input{'1', '.', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);
  auto program = parser.parse_program();
  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() == "(f64<1>)");
}

TEST_CASE("Parse literal qualifier expression. ", "[parser]") {
  std::vector<char> input{'1', 'u', '3', '2', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);
  auto program = parser.parse_program();
  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() == "(u32<1>)");
}

TEST_CASE("Parse integer addition expression. ", "[parser]") {
  std::vector<char> input{'1', '+', '2', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);
  auto program = parser.parse_program();
  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() == "(+ (i64<1>) (i64<2>))");
}

TEST_CASE("Parse integer subtraction expression. ", "[parser]") {
  std::vector<char> input{'1', '-', '2', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);
  auto program = parser.parse_program();
  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() == "(- (i64<1>) (i64<2>))");
}

TEST_CASE("Parse integer multiplication expression. ", "[parser]") {
  std::vector<char> input{'1', '*', '2', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);
  auto program = parser.parse_program();
  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() == "(* (i64<1>) (i64<2>))");
}

TEST_CASE("Parse integer division expression. ", "[parser]") {
  std::vector<char> input{'1', '/', '2', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);
  auto program = parser.parse_program();
  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() == "(/ (i64<1>) (i64<2>))");
}

TEST_CASE("Parse multiple integer addition expression. ", "[parser]") {
  std::vector<char> input{'1', '+', '2', '+', '3', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);

  auto program = parser.parse_program();

  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() ==
          "(+ (+ (i64<1>) (i64<2>)) (i64<3>))");
}

TEST_CASE("Parse term/factor precedence expression. ", "[parser]") {
  std::vector<char> input{'1', '+', '2', '/', '3', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);

  auto program = parser.parse_program();

  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() ==
          "(+ (i64<1>) (/ (i64<2>) (i64<3>)))");
}

TEST_CASE("Parse comparison without else. ", "[parser]") {
  std::vector<char> input{'1', '=', '=', '3', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);

  auto program = parser.parse_program();

  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() == "(== (i64<1>) (i64<3>))");
}

TEST_CASE("Parse condition expression. ", "[parser]") {
  std::vector<char> input{'i', 'f', ' ', '1', '<', '3', '{', '3', '}',
                          'e', 'l', 's', 'e', '{', '0', '}', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);

  auto program = parser.parse_program();

  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() ==
          "(if (< (i64<1>) (i64<3>)) then (i64<3>) otherwise (i64<0>))");
}

TEST_CASE("Parse condition without else. ", "[parser]") {
  std::vector<char> input{'i', 'f', ' ', '1', '!', '=',
                          '3', '{', '3', '}', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);

  auto program = parser.parse_program();

  auto statement = (ast::ExpressionStatement *)program->statements.front();
  REQUIRE(statement->expression->describe() ==
          "(if (!= (i64<1>) (i64<3>)) then (i64<3>))");
}

TEST_CASE("Parse functions. ", "[parser]") {
  std::vector<char> input{'f', 'u', 'n', 'c', ' ', 'a', 'd', 'd',
                          '(', ')', '{', '1', '+', '2', '}', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);

  auto program = parser.parse_program();

  auto statement = (ast::Function *)program->statements.front();
  REQUIRE(statement->describe() ==
          "(fn-def (fn-type add()  (block \n(+ (i64<1>) (i64<2>))\n))");
}

TEST_CASE("Parse functions with arguments ", "[parser]") {
  std::string input = "func add(a: i32, b: i32) { a + b }";
  input += (char)EOF;

  std::vector input_chars(input.begin(), input.end());
  Lexer lexer{&input_chars, 0};
  Parser parser(&lexer);

  auto program = parser.parse_program();

  auto statement = (ast::Function *)program->statements.front();
  REQUIRE(
      statement->describe() ==
      "(fn-def (fn-type add(a:i32, b:i32)  (block \n(+ (var a) (var b))\n))");
}

TEST_CASE("Parse variable declarations. ", "[parser]") {
  std::vector<char> input{'c', 'o', 'u', 'n', 't', '=', '0', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);

  auto program = parser.parse_program();

  auto statement = program->statements.front();
  REQUIRE(statement->describe() ==
          "(var-decl bool<count> (i64<0>))"); // this type should be corrected
                                              // by inference
}

TEST_CASE("Parse string literals. ", "[parser]") {
  std::vector<char> input{'"', 'c', 'o', 'u', 'n', 't', '"', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);

  auto program = parser.parse_program();

  auto statement = program->statements.front();
  REQUIRE(statement->describe() == "(string-literal<count>)");
}

TEST_CASE("Parse string literal escape sequences. ", "[parser]") {
  std::vector<char> input{'"', 't', 'a', 'b', '\\', 't', 'm', 'e', '"', EOF};
  Lexer lexer{&input, 0};
  Parser parser(&lexer);

  auto program = parser.parse_program();

  auto statement = program->statements.front();
  REQUIRE(statement->describe() == "(string-literal<tab\tme>)");
}
