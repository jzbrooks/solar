#pragma once

#include <sstream>
#include <vector>

#include "token.hpp"

namespace ast {
    struct Node;
    struct Expression;
    struct Statement;
    struct LiteralValueExpression;
    struct Binop;
    struct Condition;
    struct Call;
    struct FunctionPrototype;
    struct ExpressionStatement;
    struct Block;
    struct Function;

    struct Type {
        Token name;

        struct Primitive {
            static Token BOOL;
            static Token INT32;
            static Token INT64;
            static Token UINT32;
            static Token UINT64;
            static Token FLOAT32;
            static Token FLOAT64;
        };
    };

    union Value {
        bool boolean;
        unsigned int uint32;
        int int32;
        float float32;
        long int64;
        unsigned long long uint64;
        double float64;
    };

    enum class Operation {
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE,
        COMPARE_IS_EQUAL,
        COMPARE_IS_LESS,
        COMPARE_IS_LESS_OR_EQUAL,
        COMPARE_IS_GREATER,
        COMPARE_IS_GREATER_OR_EQUAL,
        COMPARE_IS_NOT_EQUAL,
    };

    struct Node {
        [[nodiscard]]
        virtual std::string describe() const = 0;
    };

    struct ExpressionVisitor {
        virtual void visit(LiteralValueExpression&) = 0;
        virtual void visit(Binop&) = 0;
        virtual void visit(Condition&) = 0;
        virtual void visit(Call&) = 0;
    };

    struct Expression : public Node {
        virtual void accept(ExpressionVisitor&) = 0;
        virtual ~Expression() = default;
    };

    struct LiteralValueExpression : public Expression {
        Type type;
        Value value;

        LiteralValueExpression() = default;
        LiteralValueExpression(Type type, const Value& value) : type(type), value(value) {}

        void accept(ExpressionVisitor& visitor) override {
            visitor.visit(*this);
        }

        [[nodiscard]]
        std::string describe() const override {
            std::ostringstream builder;

            builder << "(" << type.name.lexeme << "<";

            if (type.name.lexeme == Type::Primitive::BOOL.lexeme) {
                builder << value.boolean;
            } else if (type.name.lexeme == Type::Primitive::INT32.lexeme) {
                builder << value.int32;
            } else if (type.name.lexeme == Type::Primitive::INT64.lexeme) {
                builder << value.int64;
            } else if (type.name.lexeme == Type::Primitive::FLOAT32.lexeme) {
                builder << value.float32;
            } else if (type.name.lexeme == Type::Primitive::FLOAT64.lexeme) {
                builder << value.float64;
            } else if (type.name.lexeme == Type::Primitive::UINT32.lexeme) {
                builder << value.uint32;
            } else if (type.name.lexeme == Type::Primitive::UINT64.lexeme) {
                builder << value.uint64;
            }

            builder << ">)";
            return builder.str();
        }
    };

    struct Binop : public Expression {
        Expression* left;
        Expression* right;
        Operation operation;

        Binop(
            Expression* left,
            Expression* right,
            Operation operation
        ) : left(left), right(right), operation(operation) {}

        ~Binop() override {
            delete left;
            delete right;
        }

        void accept(ExpressionVisitor& visitor) override {
            visitor.visit(*this);
        }

        [[nodiscard]]
        std::string describe() const override {
            std::ostringstream builder;
            builder << "(";
            switch (operation) {
                case Operation::ADD:
                    builder << '+';
                    break;
                case Operation::SUBTRACT:
                    builder << '-';
                    break;
                case Operation::MULTIPLY:
                    builder << '*';
                    break;
                case Operation::DIVIDE:
                    builder << '/';
                    break;
                case Operation::COMPARE_IS_EQUAL:
                    builder << "==";
                    break;
                case Operation::COMPARE_IS_NOT_EQUAL:
                    builder << "!=";
                    break;
                case Operation::COMPARE_IS_LESS:
                    builder << "<";
                    break;
                case Operation::COMPARE_IS_LESS_OR_EQUAL:
                    builder << "<=";
                    break;
                case Operation::COMPARE_IS_GREATER:
                    builder << ">";
                    break;
                case Operation::COMPARE_IS_GREATER_OR_EQUAL:
                    builder << ">=";
                    break;
            }

            builder << " " << left->describe() << " " << right->describe() << ")";
            return builder.str();
        }
    };

    struct Condition : public Expression
    {
        Expression* condition;
        Expression* then;
        Expression* otherwise;

        Condition() = default;

        ~Condition() override
        {
            delete condition;
            delete then;
            delete otherwise;
        }

        void accept(ExpressionVisitor& visitor) override {
            visitor.visit(*this);
        }

        [[nodiscard]]
        std::string describe() const override {
            std::ostringstream builder;
            builder << "(if " << condition->describe()
                    << " then " << then->describe();
            if (otherwise)
            {
                builder << " otherwise " << otherwise->describe();
            }

            builder << ")";
            return builder.str();
        }
    };

    struct Call : public Expression {
        Token name;
        std::vector<Expression*> arguments;

        void accept(ExpressionVisitor& visitor) override {
            visitor.visit(*this);
        }

        [[nodiscard]]
        std::string describe() const override {
            std::ostringstream builder;
            builder << "(fn-call " << name.lexeme << ": ";
            for (const auto& expression : arguments) {
                builder << expression->describe();

                if (&expression != &arguments.back()) {
                    builder << ", ";
                }
            }

            builder << ")";
            return builder.str();
        }
    };

    struct StatementVisitor {
        virtual void visit(ExpressionStatement&) = 0;
        virtual void visit(FunctionPrototype&) = 0;
        virtual void visit(Function&) = 0;
        virtual void visit(Block&) = 0;
    };

    struct Statement : public Node {
        virtual void accept(StatementVisitor &) = 0;
    };

    struct ExpressionStatement : public Statement {
        Expression* expression;

        explicit ExpressionStatement(Expression* expression) : expression(expression) {}

        void accept(StatementVisitor& visitor) override {
            visitor.visit(*this);
        }

        [[nodiscard]]
        std::string describe() const override {
            return expression->describe();
        }
    };

    struct Parameter {
        Token name;
        Token type;
    };

    struct Block : public Statement {
        std::vector<Statement*> statements;

        void accept(StatementVisitor& visitor) override {
            visitor.visit(*this);
        }

        [[nodiscard]]
        std::string describe() const override {
            std::ostringstream builder;
            builder << "(block " << std::endl;
            for (const auto& statement : statements) {
                builder << statement->describe() << std::endl;
            }
            builder << ")";

            return builder.str();
        }
    };

    struct FunctionPrototype : public Statement {
        Token name;
        std::vector<Parameter> parameterList;

        void accept(StatementVisitor& visitor) override {
            visitor.visit(*this);
        }

        [[nodiscard]]
        std::string describe() const override {
            std::ostringstream builder;
            builder << "(fn-type " << name.lexeme << "(";
            for (const auto& parameter : parameterList) {
                builder << parameter.name.lexeme << ":" << parameter.type.lexeme;
                if (&parameter != &parameterList.back()) builder << ", ";
            }
            builder << ") ";

            return builder.str();
        }
    };

    struct Function : public Statement {
        FunctionPrototype* prototype;
        Block* body;

        void accept(StatementVisitor& visitor) override {
            visitor.visit(*this);
        }

        [[nodiscard]]
        std::string describe() const override {
            std::ostringstream builder;
            builder << "(fn-def "
                    << prototype->describe()
                    << " "
                    << body->describe()
                    << ")";

            return builder.str();
        }
    };

    struct Program {
        std::vector<Statement*> statements;
    };
}