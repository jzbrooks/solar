#pragma once

#include "ast.hpp"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"

#include <string>
#include <unordered_map>
#include <vector>

struct DebugInfo {
  llvm::DICompileUnit *compile_unit;
  std::vector<llvm::DIScope *> lexical_scopes;

  void emit_location(llvm::IRBuilder<> *, const ast::Node *node);
};

class ExpressionGenerator : public ast::ExpressionVisitor {
private:
  llvm::Module *module;
  llvm::IRBuilder<> *builder;
  std::unordered_map<std::string, llvm::AllocaInst *> *named_values;

  llvm::DIBuilder *debug_info_builder;
  DebugInfo *debug_info;

public:
  explicit ExpressionGenerator(
      llvm::Module *module, llvm::IRBuilder<> *builder,
      llvm::DIBuilder *debug_info_builder, DebugInfo *debug_info,
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

  DebugInfo *debug_info;
  llvm::DIBuilder *debug_info_builder;

public:
  explicit StatementGenerator(
      llvm::Module *module, llvm::IRBuilder<> *builder,
      llvm::DIBuilder *debug_info_builder, DebugInfo *debug_info,
      ExpressionGenerator &expressionGenerator,
      std::unordered_map<std::string, llvm::AllocaInst *> *named_values,
      bool release);

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

  DebugInfo *debug_info;
  llvm::DIBuilder *debug_info_builder;

public:
  CodeGen();
  ~CodeGen();
  llvm::Module *compile_module(const char *, ast::Program *,
                               bool release = false);
};