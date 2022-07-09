#include "catch/catch.hpp"

#include "../src/lexer.hpp"

TEST_CASE("Identifiers are given the appropriate kind", "[lexer]") {
  std::vector<char> input{'x'};
  Lexer lexer{&input, 0};

  REQUIRE(lexer.next().kind == Token::Kind::IDENTIFIER);
}

TEST_CASE("Plus is given the appropriate kind", "[lexer]") {
  std::vector<char> input{'+'};
  Lexer lexer{&input, 0};

  REQUIRE(lexer.next().kind == Token::Kind::PLUS);
}

TEST_CASE("Leading tabs are eaten", "[lexer]") {
  std::vector<char> input{'\t', '+'};
  Lexer lexer{&input, 0};

  REQUIRE(lexer.next().kind == Token::Kind::PLUS);
}

TEST_CASE("Leading spaces are eaten", "[lexer]") {
  std::vector<char> input{' ', ' ', '+'};
  Lexer lexer{&input, 0};

  REQUIRE(lexer.next().kind == Token::Kind::PLUS);
}

TEST_CASE("Leading newlines are eaten", "[lexer]") {
  std::vector<char> input{'\n', '+'};
  Lexer lexer{&input, 0};

  REQUIRE(lexer.next().kind == Token::Kind::PLUS);
}

TEST_CASE("Multi-letter identifier are lexed", "[lexer]") {
  std::vector<char> input{'t', 'e', 's', 't'};
  Lexer lexer{&input, 0};

  auto token = lexer.next();

  REQUIRE(token.kind == Token::Kind::IDENTIFIER);
  REQUIRE(token.lexeme == "test");
}

TEST_CASE("Identifier can begin with '_'", "[lexer]") {
  std::vector<char> input{'_', 't', 'e', 's', 't'};
  Lexer lexer{&input, 0};

  auto token = lexer.next();

  REQUIRE(token.kind == Token::Kind::IDENTIFIER);
  REQUIRE(token.lexeme == "_test");
}

TEST_CASE("Reserved words are lexed", "[lexer]") {
  std::vector<char> input{'f', 'u', 'n', 'c'};
  Lexer lexer{&input, 0};

  auto token = lexer.next();

  REQUIRE(token.kind == Token::Kind::FUNC);
  REQUIRE(token.lexeme == "func");
}

TEST_CASE("Numbers words are lexed", "[lexer]") {
  std::vector<char> input{'9', '3', '2', '1'};
  Lexer lexer{&input, 0};

  auto token = lexer.next();

  REQUIRE(token.kind == Token::Kind::NUMBER);
  REQUIRE(token.lexeme == "9321");
}

TEST_CASE("Arrows are lexed", "[lexer]") {
  std::vector<char> input{'-', '>'};
  Lexer lexer{&input, 0};

  auto token = lexer.next();

  REQUIRE(token.kind == Token::Kind::ARROW);
  REQUIRE(token.lexeme == "->");
}

TEST_CASE("Less equal comparisons are lexed", "[lexer]") {
  std::vector<char> input{'<', '='};
  Lexer lexer{&input, 0};

  auto token = lexer.next();

  REQUIRE(token.kind == Token::Kind::LESS_EQUAL);
  REQUIRE(token.lexeme == "<=");
}

TEST_CASE("Less comparisons are lexed", "[lexer]") {
  std::vector<char> input{'<'};
  Lexer lexer{&input, 0};

  auto token = lexer.next();

  REQUIRE(token.kind == Token::Kind::LESS);
  REQUIRE(token.lexeme == "<");
}

TEST_CASE("Multiple tokens are lexed", "[lexer]") {
  std::vector<char> input{'5', '>', '=', '5', '0'};
  Lexer lexer{&input, 0};

  auto first = lexer.next();

  REQUIRE(first.kind == Token::Kind::NUMBER);
  REQUIRE(first.lexeme == "5");

  auto second = lexer.next();

  REQUIRE(second.kind == Token::Kind::GREATER_EQUAL);
  REQUIRE(second.lexeme == ">=");

  auto third = lexer.next();

  REQUIRE(third.kind == Token::Kind::NUMBER);
  REQUIRE(third.lexeme == "50");
}

TEST_CASE("Less strings are lexed", "[lexer]") {
  std::vector<char> input{'"', 'h', 'i', '"'};
  Lexer lexer{&input, 0};

  auto token = lexer.next();

  REQUIRE(token.kind == Token::Kind::STRING);
  REQUIRE(token.lexeme == "\"hi\"");
}
