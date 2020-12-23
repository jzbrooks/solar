#pragma once

namespace ast {
    enum class Type {
        BOOL,
        INT,
        FLOAT,
    };

    union Value {
        bool boolean;
        int int32;
        long int64;
        double floating;
    };

    enum class Operation {
        PLUS,
    };

    struct Expression {};

    struct IntExpression : public Expression {
        Value value;
    };

    struct Binop : public Expression{
        Expression* left;
        Expression* right;
        Operation operation;
    };

    struct Statement {};

    struct ExpressionStatement : public Statement {
        Expression* expression;
    };

    struct Program {
        std::vector<Statement*> statements;
    };
}