#pragma once

#include <sstream>
#include <vector>

#include "token.hpp"

namespace ast {
struct Node;
struct Expression;
struct Statement;
struct Variable;
struct LiteralValueExpression;
struct Binop;
struct Condition;
struct Call;
struct StringLiteral;
struct VariableDeclaration;
struct FunctionPrototype;
struct ExpressionStatement;
struct Block;
struct Function;
struct Return;

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
  SourcePosition position;

  Node() = delete;
  Node(SourcePosition position) : position(position) {}

  [[nodiscard]] virtual std::string describe() const = 0;
};

struct ExpressionVisitor {
  virtual void *visit(Variable &) = 0;
  virtual void *visit(LiteralValueExpression &) = 0;
  virtual void *visit(Binop &) = 0;
  virtual void *visit(Condition &) = 0;
  virtual void *visit(Call &) = 0;
  virtual void *visit(StringLiteral &) = 0;
};

struct Expression : public Node {
  Expression() = delete;
  Expression(SourcePosition position) : Node(position) {}

  virtual void *accept(ExpressionVisitor &) = 0;
  virtual ~Expression() = default;
};

struct Variable : public Expression {
  Token name;

  Variable() = delete;
  explicit Variable(SourcePosition position, const Token &name)
      : Expression(position), name(name) {}

  void *accept(ExpressionVisitor &visitor) override {
    return visitor.visit(*this);
  }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;
    builder << "(var " << name.lexeme << ")";
    return builder.str();
  }
};

struct StringLiteral : public Expression {
  std::string value;

  StringLiteral() = delete;
  explicit StringLiteral(SourcePosition position, std::string value)
      : Expression(position), value(std::move(value)) {}

  void *accept(ExpressionVisitor &visitor) override {
    return visitor.visit(*this);
  }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;
    builder << "(string-literal<" << value << ">)";
    return builder.str();
  }
};

struct LiteralValueExpression : public Expression {
  Type type;
  Value value;

  LiteralValueExpression() = delete;
  LiteralValueExpression(SourcePosition position, Type type, Value value)
      : Expression(position), type(std::move(type)), value(value) {}

  void *accept(ExpressionVisitor &visitor) override {
    return visitor.visit(*this);
  }

  [[nodiscard]] std::string describe() const override {
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
  Expression *left;
  Expression *right;
  Operation operation;

  Binop(SourcePosition position, Expression *left, Expression *right,
        Operation operation)
      : Expression(position), left(left), right(right), operation(operation) {}

  ~Binop() override {
    delete left;
    delete right;
  }

  void *accept(ExpressionVisitor &visitor) override {
    return visitor.visit(*this);
  }

  [[nodiscard]] std::string describe() const override {
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

struct Condition : public Expression {
  Expression *condition;
  Expression *then;
  Expression *otherwise;

  Condition() = delete;
  Condition(SourcePosition position) : Expression(position) {}

  ~Condition() override {
    delete condition;
    delete then;
    delete otherwise;
  }

  void *accept(ExpressionVisitor &visitor) override {
    return visitor.visit(*this);
  }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;
    builder << "(if " << condition->describe() << " then " << then->describe();
    if (otherwise) {
      builder << " otherwise " << otherwise->describe();
    }

    builder << ")";
    return builder.str();
  }
};

struct Call : public Expression {
  Token name;
  std::vector<Expression *> arguments;

  explicit Call(SourcePosition position, const Token &name,
                std::vector<Expression *> arguments)
      : Expression(position), name(name), arguments(std::move(arguments)){};

  ~Call() override {
    for (auto argument : arguments) {
      delete argument;
    }
  }

  void *accept(ExpressionVisitor &visitor) override {
    return visitor.visit(*this);
  }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;
    builder << "(fn-call " << name.lexeme << ": ";
    for (const auto &expression : arguments) {
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
  virtual void visit(VariableDeclaration &) = 0;
  virtual void visit(ExpressionStatement &) = 0;
  virtual void visit(FunctionPrototype &) = 0;
  virtual void visit(Function &) = 0;
  virtual void visit(Block &) = 0;
  virtual void visit(Return &) = 0;
};

struct Statement : public Node {
  Statement() = delete;
  Statement(SourcePosition position) : Node(position) {}
  virtual void accept(StatementVisitor &) = 0;
};

struct VariableDeclaration : public Statement {
  Token name;
  Token type;
  Expression *initializer;

  VariableDeclaration() = delete;
  VariableDeclaration(SourcePosition position, const Token &name,
                      const Token &type, Expression *initializer)
      : Statement(position), name(name), type(type), initializer(initializer) {}

  void accept(StatementVisitor &visitor) override { visitor.visit(*this); }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;
    builder << "(var-decl " << type.lexeme << "<" << name.lexeme << "> "
            << initializer->describe() << ")";
    return builder.str();
  }
};

struct ExpressionStatement : public Statement {
  Expression *expression;

  explicit ExpressionStatement(Expression *expression)
      : Statement(expression->position), expression(expression) {}

  void accept(StatementVisitor &visitor) override { visitor.visit(*this); }

  [[nodiscard]] std::string describe() const override {
    return expression->describe();
  }
};

struct Parameter {
  SourcePosition position;
  Token name;
  Token type;

  Parameter(const Token &name, const Token &type)
      : position(name.position), name(name), type(type) {}
};

struct Block : public Statement {
  std::vector<Statement *> statements;

  Block() = delete;
  Block(SourcePosition position) : Statement(position) {}

  void accept(StatementVisitor &visitor) override { visitor.visit(*this); }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;
    builder << "(block " << std::endl;
    for (const auto &statement : statements) {
      builder << statement->describe() << std::endl;
    }
    builder << ")";

    return builder.str();
  }
};

struct FunctionPrototype : public Statement {
  Token name;
  std::vector<Parameter> parameter_list;
  Token return_type;

  FunctionPrototype() = delete;
  FunctionPrototype(SourcePosition position) : Statement(position) {}

  void accept(StatementVisitor &visitor) override { visitor.visit(*this); }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;
    builder << "(fn-type " << name.lexeme << "(";
    for (const auto &parameter : parameter_list) {
      builder << parameter.name.lexeme << ":" << parameter.type.lexeme;
      if (&parameter != &parameter_list.back())
        builder << ", ";
    }
    builder << ") ";

    return builder.str();
  }
};

struct Function : public Statement {
  FunctionPrototype *prototype;
  Block *body;

  Function() = delete;
  Function(SourcePosition position) : Statement(position) {}

  void accept(StatementVisitor &visitor) override { visitor.visit(*this); }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;
    builder << "(fn-def " << prototype->describe() << " " << body->describe()
            << ")";

    return builder.str();
  }
};

struct Return : public Statement {
  Expression *return_value;

  Return() = delete;
  explicit Return(SourcePosition position, Expression *return_value)
      : Statement(position), return_value(return_value) {}

  void accept(StatementVisitor &visitor) override {
    return visitor.visit(*this);
  }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;
    builder << "(return " << return_value->describe() << ")";
    return builder.str();
  }
};

struct Program {
  std::vector<Statement *> statements;
};
} // namespace ast