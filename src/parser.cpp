#include "parser.hpp"
#include <sstream>

Parser::Parser(Lexer* lexer) : rules {
    {Token::Kind::NUMBER, ParseRule{&Parser::number, nullptr, Precedence::LOWEST} },
    {Token::Kind::IDENTIFIER, ParseRule{&Parser::variable, nullptr, Precedence::LOWEST} },
    {Token::Kind::PLUS, ParseRule{nullptr, &Parser::binary, Precedence::TERM} },
    {Token::Kind::MINUS, ParseRule{nullptr, &Parser::binary, Precedence::TERM} },
    {Token::Kind::STAR, ParseRule{nullptr, &Parser::binary, Precedence::FACTOR} },
    {Token::Kind::SLASH, ParseRule{nullptr, &Parser::binary, Precedence::FACTOR} },
    {Token::Kind::END, ParseRule{nullptr, nullptr, Precedence::LOWEST} },
}, lexer { lexer } {
}

ast::Program* Parser::parseProgram()
{
    advance(); // prime the pump
    advance();

    std::vector<ast::Statement*> statements;
    while (current.kind != Token::Kind::END)
    {
        auto expression = this->expression(Precedence::LOWEST);
        auto statement = new ast::ExpressionStatement { .expression = expression };
        statements.emplace_back(statement);
        advance();
    }

    return new ast::Program { statements };
}

ast::Expression* Parser::expression(Precedence precedence)
{
    auto prefixRule = rules[current.kind].prefix;
    if (prefixRule == nullptr) {
        printf("Expected a prefix parse rule for token kind: %s", name(current.kind));
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

ast::Expression* Parser::number() {
    std::string slice(lexer->input->data() + current.start, current.length);

    auto expression = new ast::LiteralValueExpression();
    if (slice.find('.') != std::string::npos) {
        if (slice.ends_with("f32")) {
            expression->type = ast::Type::FLOAT32;
            expression->value = ast::Value{.float32 = std::stof(slice)};
        } else {
            expression->type = ast::Type::FLOAT64;
            expression->value = ast::Value{.float64 = std::stod(slice)};
        }
    } else {
        if (slice.ends_with("i32")) {
            expression->type = ast::Type::INT32;
            expression->value = ast::Value { .int32 = std::stoi(slice) };
        } else if (slice.ends_with("u32")) {
            expression->type = ast::Type::UINT32;
            expression->value = ast::Value { .uint32 = (unsigned int)std::stoul(slice) };
        } else if (slice.ends_with("u64")) {
            expression->type = ast::Type::UINT64;
            expression->value = ast::Value { .uint64 = std::stoul(slice) };
        } else {
            expression->type = ast::Type::INT64;
            expression->value = ast::Value { .int64 = std::stol(slice) };
        }
    }

    return expression;
}
ast::Expression* Parser::variable() {
    return new ast::LiteralValueExpression(ast::Type::BOOL, ast::Value { .boolean = false });
}
ast::Expression* Parser::binary(ast::Expression* left) {
    ast::Operation operation;

    switch(current.kind)
    {
        case Token::Kind::PLUS:
            operation = ast::Operation::ADD;
            break;
        case Token::Kind::MINUS:
            operation = ast::Operation::SUBTRACT;
            break;
        case Token::Kind::STAR:
            operation = ast::Operation::MULTIPLY;
            break;
        case Token::Kind::SLASH:
            operation = ast::Operation::DIVIDE;
            break;
        default:
            std::ostringstream stream;
            stream << "Unsupported binary operation: " << name(current.kind) << std::endl;
            error(stream.str().c_str());
            // todo: panic mode
    }

    auto currentPrecedence = rules[current.kind].precedence;
    advance();
    auto right = expression(currentPrecedence);
    return new ast::Binop(left, right, operation);
}

void Parser::advance()
{
    current = lookahead;
    lookahead = lexer->next();
}

void Parser::consume(Token::Kind kind, const char* message) {
    if (current.kind == kind) {
        advance();
        return;
    }

    std::ostringstream stream;
    stream << "Expected " << name(kind) << ", but got " << name(current.kind) << std::endl << std::string(message);
    error(stream.str().c_str());
}

void Parser::error(const char* message) const
{
    error(this->current, message);
}

void Parser::error(const Token& token, const char* message) const
{
    fprintf(stderr, "[line %d] Error", token.line);
    fprintf(stderr, " at %d:", token.start);
    fprintf(stderr, " %s\n", message);
}
