#pragma once

#include <sstream>

namespace ast {
    enum class Type {
        BOOL,
        INT32,
        INT64,
        FLOAT32,
        FLOAT64,
        UINT32,
        UINT64,
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
    };

    struct Expression {
        [[nodiscard]]
        virtual std::string describe() const = 0;
        virtual ~Expression() = default;
    };

    struct LiteralValueExpression : public Expression {
        Type type;
        Value value;

        LiteralValueExpression() = default;
        LiteralValueExpression(Type type, const Value& value) : type(type), value(value) {}

        [[nodiscard]]
        std::string describe() const override {
            std::ostringstream builder;

            builder << "(";

            switch(this->type) {
                case Type::BOOL:
                    builder << "bool<" << value.boolean << ">";
                    break;
                case Type::INT32:
                    builder << "int32<" << value.int32 << ">";
                    break;
                case Type::INT64:
                    builder << "int64<" << value.int64 << ">";
                    break;
                case Type::FLOAT32:
                    builder << "float32<" << value.float32 << ">";
                    break;
                case Type::FLOAT64:
                    builder << "float64<" << value.float64 << ">";
                    break;
                case Type::UINT32:
                    builder << "uint32<" << value.uint32 << ">";
                    break;
                case Type::UINT64:
                    builder << "uint64<" << value.uint64 << ">";
                    break;
            }

            builder << ")";
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
            }

            builder << " " << left->describe() << " " << right->describe() << ")";
            return builder.str();
        }
    };

    struct Statement {};

    struct ExpressionStatement : public Statement {
        Expression* expression;
    };

    struct Program {
        std::vector<Statement*> statements;
    };
}