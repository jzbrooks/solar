#include "codegen.hpp"

#include <cassert>

using namespace llvm;

Module* CodeGen::compileModule(const char* id, ast::Program* program) {
    auto module = new Module(id, *context);
    StatementGenerator statementGenerator(module);

    for (const auto& statement : program->statements) {
        statement->accept(statementGenerator);
    }

    return module;
}

CodeGen::CodeGen() {
    context = new LLVMContext();
}

StatementGenerator::StatementGenerator(Module* module) : module(module) {}

void StatementGenerator::visit(ast::FunctionDeclaration& node) {
    // not implemented
}

void StatementGenerator::visit(ast::ExpressionStatement& node) {

}

ExpressionGenerator::ExpressionGenerator(llvm::Module* module) : module(module) {
    builder = new llvm::IRBuilder(module->getContext());
}

void ExpressionGenerator::visit(ast::LiteralValueExpression &expression) {

}

void ExpressionGenerator::visit(ast::Binop &binop) {
    // todo: robustness
    auto left = (ast::LiteralValueExpression*)binop.left;
    auto right = (ast::LiteralValueExpression*)binop.right;
    auto operation = binop.operation;

    if (left->type.name == ast::Type::Primitive::INT64) {
        auto leftValue = ConstantInt::get(Type::getInt64Ty(module->getContext()), left->value.int64);
        auto rightValue = ConstantInt::get(Type::getInt64Ty(module->getContext()), right->value.int64);
        switch (operation) {
            case ast::Operation::ADD:
                builder->CreateAdd(leftValue, rightValue);
                break;
            default:
                assert(0);
        }
    }
}

void ExpressionGenerator::visit(ast::Condition &condition) {

}
