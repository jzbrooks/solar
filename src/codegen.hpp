#pragma once

#include "ast.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"

#include <string>
#include <unordered_map>

class ExpressionGenerator : public ast::ExpressionVisitor {
private:
  llvm::Module *module;
  llvm::IRBuilder<> *builder;
  std::unordered_map<std::string, llvm::AllocaInst *> *named_values;

public:
  explicit ExpressionGenerator(
      llvm::Module *module, llvm::IRBuilder<> *builder,
      std::unordered_map<std::string, llvm::AllocaInst *> *named_values);

  void *visit(ast::Variable &variable) override;
  void *visit(ast::LiteralValueExpression &expression) override;
  void *visit(ast::Binop &binop) override;
  void *visit(ast::Condition &condition) override;
  void *visit(ast::Call &call) override;
  void *visit(ast::StringLiteral &) override;
};

class StatementGenerator : public ast::StatementVisitor {
private:
  llvm::Module *module;
  llvm::IRBuilder<> *builder;
  ExpressionGenerator &expressionGenerator;
  std::unordered_map<std::string, llvm::AllocaInst *> *named_values;
  llvm::legacy::FunctionPassManager *function_pass_manager;

public:
  explicit StatementGenerator(
      llvm::Module *module, llvm::IRBuilder<> *builder,
      ExpressionGenerator &expressionGenerator,
      std::unordered_map<std::string, llvm::AllocaInst *> *named_values);

  virtual ~StatementGenerator() { delete function_pass_manager; }

public:
  void visit(ast::VariableDeclaration &statement) override;
  void visit(ast::FunctionPrototype &statement) override;
  void visit(ast::ExpressionStatement &statement) override;
  void visit(ast::Function &function) override;
  void visit(ast::Block &block) override;
  void visit(ast::Return &) override;
};

class CodeGen {
  llvm::LLVMContext *context;
  llvm::IRBuilder<> *builder;
  std::unordered_map<std::string, llvm::AllocaInst *> *named_values;

public:
  CodeGen();
  ~CodeGen();
  llvm::Module *compile_module(const char *, ast::Program *);
};