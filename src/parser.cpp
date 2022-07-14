#include "parser.hpp"
#include <cassert>
#include <sstream>

using namespace ast;
using namespace std;

Parser::Parser(Lexer* lexer)
    : rules {
        { Token::Kind::END, ParseRule { nullptr, nullptr, Precedence::NONE } },
        { Token::Kind::EQUAL, ParseRule { nullptr, &Parser::binary, Precedence::EQUALS } },
        { Token::Kind::FUNC, ParseRule { &Parser::function, nullptr, Precedence::NONE } },
        { Token::Kind::GREATER, ParseRule { nullptr, &Parser::binary, Precedence::INEQUALITY } },
        { Token::Kind::GREATER_EQUAL, ParseRule { nullptr, &Parser::binary, Precedence::INEQUALITY } },
        { Token::Kind::IDENTIFIER, ParseRule { &Parser::variable, nullptr, Precedence::NONE } },
        { Token::Kind::IF, ParseRule { &Parser::conditional, nullptr, Precedence::NONE } },
        { Token::Kind::LESS, ParseRule { nullptr, &Parser::binary, Precedence::INEQUALITY } },
        { Token::Kind::LESS_EQUAL, ParseRule { nullptr, &Parser::binary, Precedence::INEQUALITY } },
        { Token::Kind::LPAREN, ParseRule { &Parser::grouping, &Parser::call, Precedence::CALL } },
        { Token::Kind::MINUS, ParseRule { nullptr, &Parser::binary, Precedence::TERM } },
        { Token::Kind::NOT_EQUAL, ParseRule { nullptr, &Parser::binary, Precedence::EQUALS } },
        { Token::Kind::NUMBER, ParseRule { &Parser::number, nullptr, Precedence::NONE } },
        { Token::Kind::PLUS, ParseRule { nullptr, &Parser::binary, Precedence::TERM } },
        { Token::Kind::STAR, ParseRule { nullptr, &Parser::binary, Precedence::FACTOR } },
        { Token::Kind::SLASH, ParseRule { nullptr, &Parser::binary, Precedence::FACTOR } },
        { Token::Kind::STRING, ParseRule { &Parser::str, nullptr, Precedence::NONE } },
        { Token::Kind::RETURN, ParseRule { &Parser::ret, nullptr, Precedence::NONE } },
    }
    , lexer { lexer }
    , errors(vector<string>())
{
}

Program *Parser::parse_program() {
  advance(); // prime the pump

  vector<Statement *> statements;
  while (current.kind != Token::Kind::END) {
    statements.emplace_back((Statement *)statement());
    advance();
  }

  return new Program{statements};
}

Node *Parser::expression(Precedence precedence) {
  advance();
  auto prefixRule = rules[previous.kind].prefix;
  if (!prefixRule) {
    errors.emplace_back("Expected a prefix parse rule for token kind: " +
                        string(name(previous.kind)));
    return nullptr;
  }

  auto left = (this->*(prefixRule))();

  while (precedence <= rules[current.kind].precedence) {
    advance();

    auto infixRule = rules[previous.kind].infix;
    if (!infixRule) {
      return left;
    }

    left = (this->*(infixRule))(left);
  }

  return left;
}

Node *Parser::conditional() {
  auto expression = new Condition;

  consume(Token::Kind::IF, "Expected an if keyword");
  expression->condition =
      (Expression *)this->expression(Precedence::ASSIGNMENT);
  consume(Token::Kind::LBRACE, "'{' expected after if condition.");
  expression->then = (Expression *)this->expression(Precedence::ASSIGNMENT);
  consume(Token::Kind::RBRACE, "'}' expected after if body.");

  if (current.kind == Token::Kind::ELSE) {
    consume(Token::Kind::ELSE, "Expected an else keyword.");
    consume(Token::Kind::LBRACE, "'{' expected after else.");
    expression->otherwise =
        (Expression *)this->expression(Precedence::ASSIGNMENT);
    consume(Token::Kind::RBRACE, "'}' expected after else body.");
  } else {
    expression->otherwise = nullptr;
  }

  return expression;
}

Node *Parser::number() { // NOLINT(readability-make-member-function-const)
  Type type;
  Value value;

  auto lexeme = previous.lexeme;
  if (lexeme.find('.') != string::npos) {
    if (lexeme.ends_with("f32")) {
      type = Type{Type::Primitive::FLOAT32};
      value = Value{.float32 = stof(lexeme)};
    } else {
      type = Type{Type::Primitive::FLOAT64};
      value = Value{.float64 = stod(lexeme)};
    }
  } else {
    if (lexeme.ends_with("i32")) {
      type = Type{Type::Primitive::INT32};
      value = Value{.int32 = stoi(lexeme)};
    } else if (lexeme.ends_with("u32")) {
      type = Type{Type::Primitive::UINT32};
      value = Value{.uint32 = (unsigned int)stoul(lexeme)};
    } else if (lexeme.ends_with("u64")) {
      type = Type{Type::Primitive::UINT64};
      value = Value{.uint64 = stoul(lexeme)};
    } else {
      type = Type{Type::Primitive::INT64};
      value = Value{.int64 = stol(lexeme)};
    }
  }

  return new LiteralValueExpression(type, value);
}

Node *Parser::variable() { return new Variable(previous); }

Node *Parser::ret() {
  consume(Token::Kind::RETURN, "Expected a return keyword");

  auto value = dynamic_cast<Expression *>(expression(Precedence::ASSIGNMENT));

  return new Return(value);
}

Node *Parser::str() {
  auto string = previous.lexeme.substr(1, previous.lexeme.size() - 2);
  std::ostringstream string_with_replacements;

  for (auto i = 0; i < string.size(); ++i) {
    if (string[i] != '\\') {
      string_with_replacements << string[i];
    } else {
      if (i + 1 < string.size()) {
        error("Incomplete character escape sequence in string");
      }

      switch (string[i + 1]) {
      case '0':
        string_with_replacements << (char)0x0;
        break;
      case 't':
        string_with_replacements << (char)0x9;
        break;
      case 'n':
        string_with_replacements << (char)0xA;
        break;
      case 'r':
        string_with_replacements << (char)0xD;
        break;
      default:
        std::string message = "Unknown character escape sequence in string (";
        message += string[i];
        message += string[i + 1];
        message += ')';
        error(message);
      }
      ++i;
    }
  }

  return new StringLiteral(string_with_replacements.str());
}

Node *Parser::binary(Node *left) {
  Operation operation;

  switch (previous.kind) {
  case Token::Kind::PLUS:
    operation = Operation::ADD;
    break;
  case Token::Kind::MINUS:
    operation = Operation::SUBTRACT;
    break;
  case Token::Kind::STAR:
    operation = Operation::MULTIPLY;
    break;
  case Token::Kind::SLASH:
    operation = Operation::DIVIDE;
    break;
  case Token::Kind::LESS:
    operation = Operation::COMPARE_IS_LESS;
    break;
  case Token::Kind::LESS_EQUAL:
    operation = Operation::COMPARE_IS_LESS_OR_EQUAL;
    break;
  case Token::Kind::GREATER:
    operation = Operation::COMPARE_IS_GREATER;
    break;
  case Token::Kind::GREATER_EQUAL:
    operation = Operation::COMPARE_IS_GREATER_OR_EQUAL;
    break;
  case Token::Kind::EQUAL:
    operation = Operation::COMPARE_IS_EQUAL;
    break;
  case Token::Kind::NOT_EQUAL:
    operation = Operation::COMPARE_IS_NOT_EQUAL;
    break;
  default:
    ostringstream stream;
    stream << "Unsupported binary operation: " << name(previous.kind) << endl;
    error(stream.str());
    return nullptr;
  }

  auto previousPrecedence = rules[previous.kind].precedence;
  auto right = dynamic_cast<Expression *>(
      expression((Precedence)((int)previousPrecedence + 1)));
  return new Binop(dynamic_cast<Expression *>(left), right, operation);
}

Node *Parser::function() {
  auto type = new FunctionPrototype;
  consume(Token::Kind::FUNC, "Expected a func keyword");
  assert(current.kind == Token::Kind::IDENTIFIER);
  type->name = current;
  type->parameter_list = vector<Parameter>();
  advance();
  consume(Token::Kind::LPAREN, "Expected '('");
  while (current.kind != Token::Kind::RPAREN) {
    if (!type->parameter_list.empty() && current.kind == Token::Kind::COMMA)
      advance();

    auto parameter_name = current;
    consume(Token::Kind::IDENTIFIER,
            "Expected a name for a function parameter");
    consume(Token::Kind::COLON,
            "Expected a colon after function parameter name");
    auto parameter_type = current;
    consume(Token::Kind::IDENTIFIER,
            "Expected a type name for a function parameter");
    type->parameter_list.emplace_back(parameter_name, parameter_type);
  }
  consume(Token::Kind::RPAREN, "Expected ')'");
  auto return_type = Token(Token::Kind::IDENTIFIER, "Void");
  if (current.kind == Token::Kind::ARROW) {
    advance();
    return_type = current;
    consume(Token::Kind::IDENTIFIER, "Expected a return type");
  }
  type->return_type = return_type;

  auto function = new Function;
  function->prototype = type;
  function->body = (Block *)block();
  return function;
}

Node *Parser::block() {
  auto block = new Block;
  block->statements = vector<Statement *>();
  consume(Token::Kind::LBRACE, "Expected a '{'");
  while (current.kind != Token::Kind::RBRACE) {
    block->statements.push_back((Statement *)statement());
  }
  return block;
}

Node *Parser::statement() {
  switch (current.kind) {
  case Token::Kind::FUNC:
    return function();
  case Token::Kind::RETURN:
    return ret();
  case Token::Kind::VAR:
    return assignment();
  default:
    auto expr = (Expression *)expression(Precedence::ASSIGNMENT);
    return new ExpressionStatement(expr);
  }
}

Node *Parser::call(Node *left) {
  // todo: Is there a better way to parse function names?
  //  Will this cause problems?
  auto name = dynamic_cast<Variable *>(left);

  consume(Token::Kind::LPAREN,
          "Expected '(' at the beginning of a parameter list");

  std::vector<Expression *> argument_expressions;
  if (current.kind != Token::Kind::RPAREN) {
    // Commas aren't allowed before the first argument
    argument_expressions.push_back(
        dynamic_cast<Expression *>(expression(Precedence::ASSIGNMENT)));

    while (current.kind != Token::Kind::RPAREN) {
      if (current.kind == Token::Kind::COMMA)
        advance();

      auto argument_expression =
          dynamic_cast<Expression *>(expression(Precedence::ASSIGNMENT));
      argument_expressions.push_back(argument_expression);
    }
  }

  consume(Token::Kind::RPAREN, "Expected ')' at the end of an parameter list");

  return new ast::Call(name->name, argument_expressions);
}

ast::Node *Parser::assignment() {
  consume(Token::Kind::VAR, "Expected let for variable declaration");
  auto name = current;
  consume(Token::Kind::IDENTIFIER, "Expected a variable name");
  consume(Token::Kind::COLON,
          "Expected a colon between variable name and type");
  auto type = current;
  consume(Token::Kind::IDENTIFIER, "Expected a type name");
  consume(Token::Kind::ASSIGN, "Expected an initializer");
  auto initializer =
      dynamic_cast<Expression *>(expression(Precedence::ASSIGNMENT));

  return new VariableDeclaration(name, type, initializer);
}

ast::Node *Parser::grouping() {
  auto expr = expression(Precedence::ASSIGNMENT);
  consume(Token::Kind::RPAREN, "Expected ')' after expression");
  return expr;
}

void Parser::advance() {
  previous = current;
  current = lexer->next();
}

void Parser::consume(Token::Kind kind, const string &message) {
  if (current.kind == kind) {
    advance();
    return;
  }

  ostringstream stream;
  stream << "Expected " << name(kind) << ", but got " << name(current.kind)
         << endl
         << string(message);
  error(stream.str());
}

void Parser::error(const string &message) { error(this->current, message); }

void Parser::error(const Token &token, const string &message) {
  ostringstream builder;
  builder << "[line " << token.line << "] Error at " << token.lexeme << ": "
          << message << endl;
  errors.emplace_back(builder.str());
}
