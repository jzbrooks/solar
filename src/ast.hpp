#pragma once

#include <sstream>

namespace ast {
    struct Node;
    struct Expression;
    struct Statement;
    struct LiteralValueExpression;
    struct Binop;
    struct Condition;
    struct Call;
    struct FunctionType;
    struct ExpressionStatement;
    struct Block;
    struct Function;

    struct Type {
        std::string name;

        struct Primitive {
            static constexpr const char* BOOL    = "bool\0";
            static constexpr const char* INT32   = "int32\0";
            static constexpr const char* INT64   = "int64\0";
            static constexpr const char* UINT32  = "uint32\0";
            static constexpr const char* UINT64  = "uint64\0";
            static constexpr const char* FLOAT32 = "float32\0";
            static constexpr const char* FLOAT64 = "float64\0";
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

            builder << "(" << this->type.name << "<";

            if (type.name == Type::Primitive::BOOL) {
                builder << value.boolean;
            } else if (type.name == Type::Primitive::INT32) {
                builder << value.int32;
            } else if (type.name == Type::Primitive::INT64) {
                builder << value.int64;
            } else if (type.name == Type::Primitive::FLOAT32) {
                builder << value.float32;
            } else if (type.name == Type::Primitive::FLOAT64) {
                builder << value.float64;
            } else if (type.name == Type::Primitive::UINT32) {
                builder << value.uint32;
            } else if (type.name == Type::Primitive::UINT64) {
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
        Expression* met;
        Expression* otherwise;

        Condition() = default;

        ~Condition() override
        {
            delete condition;
            delete met;
            delete otherwise;
        }

        void accept(ExpressionVisitor& visitor) override {
            visitor.visit(*this);
        }

        [[nodiscard]]
        std::string describe() const override {
            std::ostringstream builder;
            builder << "(if " << condition->describe()
                    << " then " << met->describe();
            if (otherwise)
            {
                builder << " otherwise " << otherwise->describe();
            }

            builder << ")";
            return builder.str();
        }
    };

    struct Call : public Expression {
        std::string name;
        std::vector<Expression*> arguments;

        void accept(ExpressionVisitor& visitor) override {
            visitor.visit(*this);
        }

        [[nodiscard]]
        std::string describe() const override {
            std::ostringstream builder;
            builder << "(fn-call " << name << ": ";
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
        virtual void visit(FunctionType&) = 0;
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
        std::string name;
        std::string type;
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

    struct FunctionType : public Statement {
        std::string name;
        std::vector<Parameter> parameterList;

        void accept(StatementVisitor& visitor) override {
            visitor.visit(*this);
        }

        [[nodiscard]]
        std::string describe() const override {
            std::ostringstream builder;
            builder << "(fn-type " << name << "(";
            for (const auto& parameter : parameterList) {
                builder << parameter.name << ":" << parameter.type;
                if (&parameter != &parameterList.back()) builder << ", ";
            }
            builder << ") ";

            return builder.str();
        }
    };

    struct Function : public Statement {
        FunctionType* prototype;
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