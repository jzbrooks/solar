#include "parser.hpp"
#include <sstream>
#include <cassert>

using namespace ast;
using namespace std;

Parser::Parser(Lexer* lexer) : rules {
    {Token::Kind::END, ParseRule{nullptr, nullptr, Precedence::LOWEST} },
    {Token::Kind::EQUAL, ParseRule{nullptr, &Parser::binary, Precedence::EQUALS} },
    {Token::Kind::FUNC, ParseRule{&Parser::function, nullptr, Precedence::LOWEST} },
    {Token::Kind::GREATER, ParseRule{nullptr, &Parser::binary, Precedence::INEQUALITY}},
    {Token::Kind::GREATER_EQUAL, ParseRule{nullptr, &Parser::binary, Precedence::INEQUALITY}},
    {Token::Kind::IDENTIFIER, ParseRule{&Parser::variable, nullptr, Precedence::LOWEST} },
    {Token::Kind::IF, ParseRule{&Parser::conditional, nullptr, Precedence::LOWEST}},
    {Token::Kind::LESS, ParseRule{nullptr, &Parser::binary, Precedence::INEQUALITY}},
    {Token::Kind::LESS_EQUAL, ParseRule{nullptr, &Parser::binary, Precedence::INEQUALITY}},
    {Token::Kind::MINUS, ParseRule{nullptr, &Parser::binary, Precedence::TERM} },
    {Token::Kind::NOT_EQUAL, ParseRule{nullptr, &Parser::binary, Precedence::EQUALS} },
    {Token::Kind::NUMBER, ParseRule{&Parser::number, nullptr, Precedence::LOWEST} },
    {Token::Kind::PLUS, ParseRule{nullptr, &Parser::binary, Precedence::TERM} },
    {Token::Kind::STAR, ParseRule{nullptr, &Parser::binary, Precedence::FACTOR} },
    {Token::Kind::SLASH, ParseRule{nullptr, &Parser::binary, Precedence::FACTOR} },
}, lexer { lexer }, errors(vector<string>()) {
}

Program* Parser::parseProgram()
{
    advance(); // prime the pump
    advance();

    vector<Statement*> statements;
    while (current.kind != Token::Kind::END)
    {
        auto expression = (Expression*)this->expression(Precedence::LOWEST);
        auto statement = new ExpressionStatement(expression);
        statements.emplace_back(statement);
        advance();
    }

    return new Program { statements };
}

Node* Parser::conditional()
{
    auto expression = new Condition;

    consume(Token::Kind::IF, "Expected an if keyword");
    expression->condition = (Expression*)this->expression(Precedence::LOWEST);
    advance();
    consume(Token::Kind::LBRACE, "'{' expected after if condition.");
    expression->then = (Expression*)this->expression(Precedence::LOWEST);
    advance();
    consume(Token::Kind::RBRACE, "'}' expected after if body.");

    if (current.kind == Token::Kind::ELSE) {
        consume(Token::Kind::ELSE, "Expected an else keyword.");
        consume(Token::Kind::LBRACE, "'{' expected after else.");
        expression->otherwise = (Expression*)this->expression(Precedence::LOWEST);
        advance();
        consume(Token::Kind::RBRACE, "'}' expected after else body.");
    } else {
        expression->otherwise = nullptr;
    }

    return expression;
}

Node* Parser::expression(Precedence precedence)
{
    auto prefixRule = rules[current.kind].prefix;
    if (!prefixRule) {
        errors.emplace_back("Expected a prefix parse rule for token kind: " + string(name(current.kind)));
        return nullptr;
    }

    auto left = (this->*(prefixRule))();

    while (precedence < rules[lookahead.kind].precedence) {
        auto infixRule = rules[lookahead.kind].infix;
        if (!infixRule) {
            return left;
        }

        advance();
        left = (this->*(infixRule))(left);
    }

    return left;
}

Node* Parser::number() { // NOLINT(readability-make-member-function-const)
    Type type;
    Value value;

    if (current.lexeme.find('.') != string::npos) {
        if (current.lexeme.ends_with("f32")) {
            type = Type { Type::Primitive::FLOAT32 };
            value = Value{.float32 = stof(current.lexeme)};
        } else {
            type = Type { Type::Primitive::FLOAT64 };
            value = Value{.float64 = stod(current.lexeme)};
        }
    } else {
        if (current.lexeme.ends_with("i32")) {
            type = Type { Type::Primitive::INT32 };
            value = Value { .int32 = stoi(current.lexeme) };
        } else if (current.lexeme.ends_with("u32")) {
            type = Type { Type::Primitive::UINT32 };
            value = Value { .uint32 = (unsigned int)stoul(current.lexeme) };
        } else if (current.lexeme.ends_with("u64")) {
            type = Type { Type::Primitive::UINT64 };
            value = Value { .uint64 = stoul(current.lexeme) };
        } else {
            type = Type { Type::Primitive::INT64 };
            value = Value { .int64 = stol(current.lexeme) };
        }
    }

    return new LiteralValueExpression(type, value);
}

Node* Parser::variable() {
    auto var = current;
    consume(Token::Kind::IDENTIFIER, "Expected an identifier.");
    return new Variable(var);
}

Node* Parser::binary(Node* left) {
    Operation operation;

    switch(current.kind)
    {
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
            stream << "Unsupported binary operation: " << name(current.kind) << endl;
            error(stream.str());
            return nullptr;
    }

    auto currentPrecedence = rules[current.kind].precedence;
    advance();
    auto right = (Expression*)expression(currentPrecedence);
    return new Binop((Expression*)left, right, operation);
}

Node* Parser::function() {
    auto type = new FunctionPrototype;
    consume(Token::Kind::FUNC, "Expected a func keyword");
    assert(current.kind == Token::Kind::IDENTIFIER);
    type->name = current;
    type->parameterList = vector<Parameter>();
    advance();
    consume(Token::Kind::LPAREN, "Expected '('");
    consume(Token::Kind::RPAREN, "Expected ')'");
    auto function = new Function;
    function->prototype = type;
    function->body = (Block*)block();
    return function;
}

Node* Parser::block() {
    auto block = new Block;
    block->statements = vector<Statement*>();
    consume(Token::Kind::LBRACE, "Expected a '{'");
    while (current.kind != Token::Kind::RBRACE) {
        block->statements.push_back((Statement*)statement());
        advance();
    }
    return block;
}

Node* Parser::statement() {
    // todo: useful until variable decls exist
    return new ExpressionStatement( (Expression*)expression(Precedence::LOWEST));
}

void Parser::advance()
{
    current = lookahead;
    lookahead = lexer->next();
}

void Parser::consume(Token::Kind kind, const string& message) {
    if (current.kind == kind) {
        advance();
        return;
    }

    ostringstream stream;
    stream << "Expected " << name(kind) << ", but got " << name(current.kind) << endl << string(message);
    error(stream.str());
}

void Parser::error(const string& message)
{
    error(this->current, message);
}

void Parser::error(const Token& token, const string& message)
{
    ostringstream builder;
    builder << "[line " << token.line << "] Error at " << token.lexeme << ": " << message << endl;
    errors.emplace_back(builder.str());
}
