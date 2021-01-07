#include "codegen.hpp"

#include <cassert>

using namespace llvm;
using namespace std;

Module* CodeGen::compileModule(const char* id, ast::Program* program) {
    auto module = new Module(id, *context);
    ExpressionGenerator expressionGenerator(module, builder, named_values);
    StatementGenerator statementGenerator(module, builder, expressionGenerator, named_values);

    for (const auto& statement : program->statements) {
        statement->accept(statementGenerator);
    }

    return module;
}

CodeGen::CodeGen() : named_values(new std::unordered_map<string, Value*>()){
    context = new LLVMContext();
    builder = new IRBuilder(*context);
}

StatementGenerator::StatementGenerator(Module* module, llvm::IRBuilder<>* builder, ExpressionGenerator& expressionGenerator, std::unordered_map<std::string, llvm::Value*>* named_values) : module(module), builder(builder), expressionGenerator(expressionGenerator), named_values(named_values) {}

void StatementGenerator::visit(ast::VariableDeclaration& node) {
    named_values->insert({node.name.lexeme, nullptr});

    if (node.initializer) {
        named_values->find(node.name.lexeme)->second = (Value*)node.initializer->accept(expressionGenerator);
    }
}

void StatementGenerator::visit(ast::FunctionPrototype& node) {
    // not implemented
}

void StatementGenerator::visit(ast::ExpressionStatement& node) {
    auto main = FunctionType::get(Type::getInt32Ty(module->getContext()), false);
    auto fn = Function::Create(main, Function::ExternalLinkage, "__anon", module);
    BasicBlock *BB = BasicBlock::Create(module->getContext(), "entry", fn);
    builder->SetInsertPoint(BB);
    auto value = (Value*)node.expression->accept(expressionGenerator);
    builder->CreateRet(value);
    verifyFunction(*fn);
}


void StatementGenerator::visit(ast::Block& block)
{

}

void StatementGenerator::visit(ast::Function& function)
{
}

ExpressionGenerator::ExpressionGenerator(llvm::Module* module, llvm::IRBuilder<>* builder, unordered_map<string, Value*>* named_values) : module(module), builder(builder), named_values(named_values) {}

void* ExpressionGenerator::visit(ast::LiteralValueExpression &expression) {
    if (expression.type.name.lexeme == ast::Type::Primitive::INT64.lexeme)
    {
        return ConstantInt::get(Type::getInt64Ty(module->getContext()), expression.value.int64);
    }

    if (expression.type.name.lexeme == ast::Type::Primitive::FLOAT64.lexeme)
    {
        return ConstantFP::get(Type::getDoubleTy(module->getContext()), expression.value.float64);
    }

    return nullptr;
}

void* ExpressionGenerator::visit(ast::Binop &binop) {
    auto left = (Value*)binop.left->accept(*this);
    auto right = (Value*)binop.right->accept(*this);
    auto operation = binop.operation;

    switch (operation) {
        case ast::Operation::ADD: {
            return builder->CreateAdd(left, right);
        }
        case ast::Operation::SUBTRACT: {
            return builder->CreateSub(left, right);
        }
        case ast::Operation::MULTIPLY: {
            return builder->CreateMul(left, right);
        }
        case ast::Operation::DIVIDE: {
            if (left->getType() == Type::getDoubleTy(module->getContext()) || right->getType() == Type::getDoubleTy(module->getContext())) {
                return builder->CreateFDiv(left, right);
            } else {
                return builder->CreateUDiv(left, right);
            }
        }
        default:
            assert(0); // todo: codegen errors
    }
}

void* ExpressionGenerator::visit(ast::Condition &condition) {
}

void* ExpressionGenerator::visit(ast::Call& call) {

}

void* ExpressionGenerator::visit(ast::Variable& variable) {
    return named_values->at(variable.name.lexeme);
}
