#include "parser.hpp"
#include <sstream>

Parser::Parser(Lexer* lexer) : rules {
    {Token::Kind::NUMBER, ParseRule{&Parser::number, nullptr, Precedence::LOWEST} },
    {Token::Kind::IDENTIFIER, ParseRule{&Parser::variable, nullptr, Precedence::LOWEST} },
    {Token::Kind::PLUS, ParseRule{nullptr, &Parser::binary, Precedence::TERM} },
}, lexer { lexer } {
}

ast::Program* Parser::parseProgram()
{
    advance(); // prime the pump

    std::vector<ast::Statement*> statements;
    while (current.kind != Token::Kind::END)
    {
        auto expression = parsePrecedence(Precedence::LOWEST);
        auto statement = new ast::ExpressionStatement { .expression = expression };
        statements.emplace_back(statement);
    }

    return new ast::Program { statements };
}

ast::Expression* Parser::parsePrecedence(Precedence precedence)
{
    auto prefixRule = rules[current.kind].prefix;
    if (prefixRule == nullptr) {
        error("Expected a prefix parse rule");
        return nullptr;
    }

    auto left = (this->*(prefixRule))();

    while (precedence <= rules[current.kind].precedence) {
        advance();
        if (current.kind == Token::Kind::END) {
            return left;
        }
        auto infixRule = rules[previous.kind].infix;

        if (infixRule != nullptr) {
            left = (this->*(infixRule))(left);
        }
    }

    return left;
}

ast::Expression* Parser::number() {
    return new ast::IntExpression { .value = ast::Value { .int32 = (int)strtol(lexer->input->data() + current.start, nullptr, 10) } };
}
ast::Expression* Parser::variable() {
    return new ast::Expression();
}
ast::Expression* Parser::binary(ast::Expression* left) {
    ast::Expression* right;
    switch(previous.kind)
    {
        case Token::Kind::PLUS:
            right = parsePrecedence(Precedence::SUM);
            return new ast::Binop { .left = left, .right = right, .operation = ast::Operation::PLUS };
        default:
            std::ostringstream stream;
            stream << "Unsupported binary operation: " << name(current.kind) << std::endl;
            error(stream.str().c_str());
            // todo: panic mode
    }
}

void Parser::advance()
{
    previous = current;
    current = lexer->next();
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
