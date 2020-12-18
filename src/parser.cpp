#include "parser.hpp"
#include <sstream>

Parser::Parser(Lexer* lexer) : rules {
    {Token::Kind::NUMBER, ParseRule{&Parser::number, nullptr, Precedence::LOWEST} },
    {Token::Kind::IDENTIFIER, ParseRule{&Parser::variable, nullptr, Precedence::LOWEST} },
    {Token::Kind::PLUS, ParseRule{nullptr, &Parser::binary, Precedence::TERM} },
}, lexer { lexer } {
}

ast::Expression* Parser::parseProgram()
{
    advance(); // prime the pump

    return parsePrecedence(Precedence::LOWEST);

    // error("Don't be dumb!");
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

        (this->*(infixRule))(left);
    }

    return left;
}

ast::Expression* Parser::number() {
    return new ast::IntExpression { .value = ast::Value { .int32 = atoi(lexer->input->data() + current.start) } };
}
ast::Expression* Parser::variable() {
    return new ast::Expression();
}
ast::Expression* Parser::binary(ast::Expression* left) {
    return new ast::Expression();
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
