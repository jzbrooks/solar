#pragma once

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "ast.hpp"

class StatementGenerator : public ast::StatementVisitor {
private:
    llvm::Module* module;

public:
    explicit StatementGenerator(llvm::Module* module);

public:
    void visit(ast::FunctionDeclaration& statement) override;
    void visit(ast::ExpressionStatement& statement) override;
};

class ExpressionGenerator : public ast::ExpressionVisitor {
private:
    llvm::Module* module;
    llvm::IRBuilder<>* builder;

public:
    explicit ExpressionGenerator(llvm::Module* module);

    void visit(ast::LiteralValueExpression &expression) override;
    void visit(ast::Binop &binop) override;
    void visit(ast::Condition &condition) override;
};

class CodeGen {
    llvm::LLVMContext* context;

public:
    CodeGen();
    llvm::Module* compileModule(const char*, ast::Program*);
};