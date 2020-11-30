#pragma once

#include <vector>
#include <string>

namespace ast {

    struct Type {

    };

    struct Parameter {
        std::string name;
        Type* type;
    };

    struct FunctionDecl {
        std::string name;
        std::vector<Parameter*> parameters;
        Type* returnType;
    };

    struct Statement {

    };

    struct Expression {

    };

    struct Block {
        std::vector<Statement*> statements;
    };

    struct Node {
        enum class Kind {

        };

        Kind kind;
        union {
            FunctionDecl functionDecl;
        };
    };
}
