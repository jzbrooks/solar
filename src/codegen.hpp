#pragma once

#include "ast.hpp"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class DebugInfoGenerator {
private:
  llvm::DIBuilder *debug_info_builder;
  llvm::IRBuilder<> *ir_builder;
  llvm::DICompileUnit *compile_unit;

public:
  std::vector<llvm::DIScope *> lexical_scopes;

  DebugInfoGenerator() = delete;
  explicit DebugInfoGenerator(llvm::Module *, llvm::IRBuilder<> *,
                              const std::filesystem::path &);
  ~DebugInfoGenerator();

  void attach_debug_info(const ast::Function &, llvm::Function *);
  void attach_debug_info(const ast::Parameter &, llvm::Argument *,
                         llvm::AllocaInst *, llvm::DISubprogram *);
  void attach_debug_info(const ast::VariableDeclaration &, llvm::AllocaInst *,
                         llvm::DISubprogram *);

  llvm::DIBasicType *get_type(const ast::TypeInfo &);
  void emit_location(const ast::Node *node);
  void finalize() const;
};

class ExpressionGenerator : public ast::ExpressionVisitor {
private:
  llvm::Module *module;
  llvm::IRBuilder<> *builder;
  std::unordered_map<std::string, llvm::AllocaInst *> *named_values;

  DebugInfoGenerator *debug_info_generator;

public:
  explicit ExpressionGenerator(
      llvm::Module *module, llvm::IRBuilder<> *builder,
      DebugInfoGenerator *debug_info_generator,
      std::unordered_map<std::string, llvm::AllocaInst *> *named_values);

  void *visit(ast::Variable &) override;
  void *visit(ast::LiteralValueExpression &) override;
  void *visit(ast::Binop &) override;
  void *visit(ast::Condition &) override;
  void *visit(ast::Call &) override;
  void *visit(ast::StringLiteral &) override;
};

class StatementGenerator : public ast::StatementVisitor {
private:
  llvm::Module *module;
  llvm::IRBuilder<> *builder;
  ExpressionGenerator &expressionGenerator;
  std::unordered_map<std::string, llvm::AllocaInst *> *named_values;
  llvm::legacy::FunctionPassManager *function_pass_manager;

  DebugInfoGenerator *debug_info_generator;

public:
  explicit StatementGenerator(
      llvm::Module *module, llvm::IRBuilder<> *builder,
      DebugInfoGenerator *debug_info_generator,
      ExpressionGenerator &expressionGenerator,
      std::unordered_map<std::string, llvm::AllocaInst *> *named_values,
      bool release);

  virtual ~StatementGenerator() { delete function_pass_manager; }

public:
  void visit(ast::VariableDeclaration &) override;
  void visit(ast::ExpressionStatement &) override;
  void visit(ast::Function &) override;
  void visit(ast::Block &) override;
  void visit(ast::Return &) override;
};

class CodeGen {
  llvm::LLVMContext *context;
  llvm::IRBuilder<> *builder;
  std::unordered_map<std::string, llvm::AllocaInst *> *named_values;

  DebugInfoGenerator *debug_info_generator;

public:
  CodeGen();
  ~CodeGen();
  llvm::Module *compile_module(const std::filesystem::path &, ast::Program *,
                               bool release = false);
};