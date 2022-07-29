#pragma once

#include <sstream>
#include <utility>
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

struct TypeInfo {
  enum class Type {
    UNINITIALIZED = 0,
    VOID,
    BOOL,
    INTEGER,
    FLOAT,
  };

  Type type = Type::UNINITIALIZED;

  bool is_signed = false;
  size_t size = -1;

  [[nodiscard]] std::string describe() const {
    std::ostringstream builder;

    if (type == Type::INTEGER && !is_signed)
      builder << "u";

    builder << TypeInfo::type_name(type);

    if (type != Type::BOOL)
      builder << size;

    return builder.str();
  }

  [[nodiscard]] static auto type_name(ast::TypeInfo::Type type) -> const
      char * {
    switch (type) {
    case ast::TypeInfo::Type::UNINITIALIZED:
      return "uninitialized\0";
    case ast::TypeInfo::Type::BOOL:
      return "bool\0";
    case ast::TypeInfo::Type::INTEGER:
      return "int\0";
    case ast::TypeInfo::Type::FLOAT:
      return "float\0";
    default:
      return "UNKNOWN\0";
    }
  }
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
  explicit Node(const SourcePosition &position) : position(position) {}
  virtual ~Node() = default;

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
  explicit Expression(const SourcePosition &position) : Node(position) {}

  virtual void *accept(ExpressionVisitor &) = 0;
};

struct Variable : public Expression {
  Token name;

  Variable() = delete;
  explicit Variable(const SourcePosition &position, const Token &name)
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
  explicit StringLiteral(const SourcePosition &position, std::string value)
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
  TypeInfo type_info;
  Value value;

  LiteralValueExpression() = delete;
  LiteralValueExpression(const SourcePosition &position, TypeInfo type,
                         Value value)
      : Expression(position), type_info(type), value(value) {}

  void *accept(ExpressionVisitor &visitor) override {
    return visitor.visit(*this);
  }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;

    builder << "(" << type_info.describe() << "<";
    switch (type_info.type) {
    case TypeInfo::Type::BOOL:
      builder << value.boolean;
      break;
    case TypeInfo::Type::INTEGER:
      if (!type_info.is_signed && type_info.size == 32) {
        builder << value.int32;
      } else if (!type_info.is_signed && type_info.size == 32) {
        builder << value.uint32;
      } else if (type_info.is_signed && type_info.size == 64) {
        builder << value.int64;
      } else if (!type_info.is_signed && type_info.size == 64) {
        builder << value.uint64;
      } else {
        assert(0);
      }
      break;
    case TypeInfo::Type::FLOAT:
      if (type_info.size == 64) {
        builder << value.float64;
      } else if (type_info.size == 32) {
        builder << value.float32;
      } else {
        assert(0);
      }
      break;
    default:
      assert(0);
      break;
    }

    builder << ">)";
    return builder.str();
  }
};

struct Binop : public Expression {
  Expression *left;
  Expression *right;
  Operation operation;

  Binop(const SourcePosition &position, Expression *left, Expression *right,
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
  explicit Condition(const SourcePosition &position) : Expression(position) {}

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

  Call(const SourcePosition &position, const Token &name,
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
  virtual void visit(Function &) = 0;
  virtual void visit(Block &) = 0;
  virtual void visit(Return &) = 0;
};

struct Statement : public Node {
  Statement() = delete;
  explicit Statement(const SourcePosition &position) : Node(position) {}
  virtual void accept(StatementVisitor &) = 0;
};

struct VariableDeclaration : public Statement {
  Token name;
  TypeInfo type_info;
  Expression *initializer;

  VariableDeclaration() = delete;
  VariableDeclaration(const SourcePosition &position, const Token &name,
                      TypeInfo type, Expression *initializer)
      : Statement(position), name(name), type_info(type),
        initializer(initializer) {}

  ~VariableDeclaration() override { delete initializer; }

  void accept(StatementVisitor &visitor) override { visitor.visit(*this); }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;

    builder << "(var-decl " << type_info.describe() << "<" << name.lexeme
            << "> " << initializer->describe() << ")";

    return builder.str();
  }
};

struct ExpressionStatement : public Statement {
  Expression *expression;

  explicit ExpressionStatement(Expression *expression)
      : Statement(expression->position), expression(expression) {}
  ~ExpressionStatement() override { delete expression; }

  void accept(StatementVisitor &visitor) override { visitor.visit(*this); }

  [[nodiscard]] std::string describe() const override {
    return expression->describe();
  }
};

struct Parameter {
  SourcePosition position;
  Token name;
  TypeInfo type_info;

  Parameter(const Token &name, const TypeInfo &type)
      : position(name.position), name(name), type_info(type) {}
};

struct Block : public Statement {
  std::vector<Statement *> statements;

  Block() = delete;
  explicit Block(const SourcePosition &position) : Statement(position) {}
  ~Block() override {
    for (auto statement : statements) {
      delete statement;
    }
  }

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

struct FunctionPrototype {
  Token name;
  std::vector<Parameter> parameter_list;
  TypeInfo return_type_info;

  [[nodiscard]] std::string describe() const {
    std::ostringstream builder;
    builder << "(fn-type " << name.lexeme << "(";
    for (const auto &parameter : parameter_list) {
      builder << parameter.name.lexeme << ":" << parameter.type_info.describe();
      if (&parameter != &parameter_list.back())
        builder << ", ";
    }
    builder << ") ";

    return builder.str();
  }
};

struct Function : public Statement {
  FunctionPrototype prototype;
  Block *body = nullptr;

  Function() = delete;
  explicit Function(const SourcePosition &position) : Statement(position) {}
  ~Function() override { delete body; }

  void accept(StatementVisitor &visitor) override { visitor.visit(*this); }

  [[nodiscard]] std::string describe() const override {
    std::ostringstream builder;
    builder << "(fn-def " << prototype.describe() << " " << body->describe()
            << ")";

    return builder.str();
  }
};

struct Return : public Statement {
  Expression *return_value;

  Return() = delete;
  Return(const SourcePosition &position, Expression *return_value)
      : Statement(position), return_value(return_value) {}
  ~Return() override { delete return_value; }

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
