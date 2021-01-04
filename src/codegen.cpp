#include "codegen.hpp"

#include <cassert>

using namespace llvm;

Module* CodeGen::compileModule(const char* id, ast::Program* program) {
    auto module = new Module(id, *context);
    ExpressionGenerator expressionGenerator(module, builder);
    StatementGenerator statementGenerator(module, builder, expressionGenerator);

    for (const auto& statement : program->statements) {
        statement->accept(statementGenerator);
    }

    return module;
}

CodeGen::CodeGen() {
    context = new LLVMContext();
    builder = new IRBuilder(*context);
}

StatementGenerator::StatementGenerator(Module* module, llvm::IRBuilder<>* builder, ExpressionGenerator& expressionGenerator) : module(module), builder(builder), expressionGenerator(expressionGenerator) {}

void StatementGenerator::visit(ast::FunctionType& node) {
    // not implemented
}

void StatementGenerator::visit(ast::ExpressionStatement& node) {
    auto main = FunctionType::get(Type::getInt32Ty(module->getContext()), false);
    auto fn = Function::Create(main, Function::ExternalLinkage, "__anon", module);
    BasicBlock *BB = BasicBlock::Create(module->getContext(), "entry", fn);
    builder->SetInsertPoint(BB);
    node.expression->accept(expressionGenerator);

    // Validate the generated code, checking for consistency.
    verifyFunction(*fn);
}


void StatementGenerator::visit(ast::Block& block)
{

}

void StatementGenerator::visit(ast::Function& function)
{

}

ExpressionGenerator::ExpressionGenerator(llvm::Module* module, llvm::IRBuilder<>* builder) : module(module), builder(builder) {}

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
            case ast::Operation::ADD: {
                auto value = builder->CreateAdd(leftValue, rightValue);
                builder->CreateRet(value);
                break;
            }
            default:
                assert(0);
        }
    }
}

void ExpressionGenerator::visit(ast::Condition &condition) {

}

void ExpressionGenerator::visit(ast::Call& call) {

}
