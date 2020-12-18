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

    struct Binop {
        Expression* left;
        Expression* right;
        Operation operation;
    };
}