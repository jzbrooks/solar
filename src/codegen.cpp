#include "codegen.hpp"

#include <cassert>

using namespace llvm;
using namespace std;

CodeGen::CodeGen() : named_values(new std::unordered_map<string, Value *>()) {
  context = new LLVMContext();
  builder = new IRBuilder(*context);
}

Module *CodeGen::compile_module(const char *id, ast::Program *program) {
  auto module = new Module(id, *context);
  ExpressionGenerator expressionGenerator(module, builder, named_values);
  StatementGenerator statementGenerator(module, builder, expressionGenerator,
                                        named_values);

  // Add printf manually
  std::vector<Type *> args = {Type::getInt8PtrTy(module->getContext())};
  auto printf_type =
      FunctionType::get(Type::getInt32Ty(module->getContext()),
                        ArrayRef<Type *>(args.data(), args.size()), true);
  AttributeList attributes;
  attributes =
      attributes.addAttribute(module->getContext(), 1U, Attribute::NoAlias);
  module->getOrInsertFunction("printf", printf_type, attributes);

  for (const auto &statement : program->statements) {
    statement->accept(statementGenerator);
  }

  return module;
}

StatementGenerator::StatementGenerator(
    Module *module, llvm::IRBuilder<> *builder,
    ExpressionGenerator &expressionGenerator,
    std::unordered_map<std::string, llvm::Value *> *named_values)
    : module(module), builder(builder),
      expressionGenerator(expressionGenerator), named_values(named_values) {}

void StatementGenerator::visit(ast::VariableDeclaration &node) {
  named_values->insert({node.name.lexeme, nullptr});

  if (node.initializer) {
    named_values->find(node.name.lexeme)->second =
        (Value *)node.initializer->accept(expressionGenerator);
  }
}

void StatementGenerator::visit(ast::FunctionPrototype &node) {
  // not implemented
}

void StatementGenerator::visit(ast::ExpressionStatement &node) {
  node.expression->accept(expressionGenerator);
}

void StatementGenerator::visit(ast::Block &block) {
  for (const auto &statement : block.statements) {
    statement->accept(*this);
  }
}

void StatementGenerator::visit(ast::Function &function) {
  // todo: check for prototype in module
  std::vector<Type *> argument_types;
  for (const auto &parameter : function.prototype->parameter_list) {
    if (parameter.type == ast::Type::Primitive::FLOAT64) {
      argument_types.emplace_back(Type::getDoubleTy(module->getContext()));
    } else if (parameter.type == ast::Type::Primitive::INT32) {
      argument_types.emplace_back(Type::getInt32Ty(module->getContext()));
    }
  }

  // todo: incomplete return type handling
  auto return_type = Type::getVoidTy(module->getContext());
  if (function.prototype->return_type == ast::Type::Primitive::INT32) {
    return_type = Type::getInt32Ty(module->getContext());
  }

  auto type = FunctionType::get(
      return_type,
      ArrayRef<Type *>(argument_types.data(), argument_types.size()), false);
  auto func = Function::Create(type, GlobalValue::LinkageTypes::ExternalLinkage,
                               function.prototype->name.lexeme, module);
  auto entry = BasicBlock::Create(module->getContext(), "entry", func);
  builder->SetInsertPoint(entry);

  assert(function.prototype->parameter_list.size() == func->arg_size());

  for (auto i = 0; i < function.prototype->parameter_list.size(); ++i) {
    func->getArg(i)->setName(function.prototype->parameter_list[i].name.lexeme);
  }

  for (auto const &arg : func->args())
    (*named_values)[arg.getName().str()] = (Value *)&arg;

  function.body->accept(*this);

  // todo: long-term, this might not work? It might be better if each
  //  block were responsible for its own return statements.
  if (entry->getTerminator() == nullptr) {
    builder->CreateRetVoid();
  }

  //#ifdef DEBUG
  verifyFunction(*func);
  //#endif
}

void StatementGenerator::visit(ast::Return &return_statement) {
  auto value =
      (Value *)return_statement.return_value->accept(expressionGenerator);
  builder->CreateRet(value);
}

ExpressionGenerator::ExpressionGenerator(
    llvm::Module *module, llvm::IRBuilder<> *builder,
    unordered_map<string, Value *> *named_values)
    : module(module), builder(builder), named_values(named_values) {}

void *ExpressionGenerator::visit(ast::LiteralValueExpression &expression) {
  if (expression.type.name.lexeme == ast::Type::Primitive::INT64.lexeme) {
    return ConstantInt::get(Type::getInt64Ty(module->getContext()),
                            expression.value.int64);
  }

  if (expression.type.name.lexeme == ast::Type::Primitive::FLOAT64.lexeme) {
    return ConstantFP::get(Type::getDoubleTy(module->getContext()),
                           expression.value.float64);
  }

  return nullptr;
}

void *ExpressionGenerator::visit(ast::Binop &binop) {
  auto left = (Value *)binop.left->accept(*this);
  auto right = (Value *)binop.right->accept(*this);
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
    if (left->getType() == Type::getDoubleTy(module->getContext()) ||
        right->getType() == Type::getDoubleTy(module->getContext())) {
      return builder->CreateFDiv(left, right);
    } else {
      return builder->CreateUDiv(left, right);
    }
  }
  default:
    assert(0); // todo: codegen errors
  }
}

void *ExpressionGenerator::visit(ast::Condition &condition) { exit(100); }

void *ExpressionGenerator::visit(ast::Call &call) {
  auto function = module->getFunction(call.name.lexeme);

  // todo: codegen errors
  assert(function);
  assert(function->isVarArg() || function->arg_size() == call.arguments.size());

  std::vector<Value *> arguments;
  for (auto const &argument_expression : call.arguments) {
    arguments.push_back(
        static_cast<Value *>(argument_expression->accept(*this)));
  }

  return builder->CreateCall(function, arguments);
}

void *ExpressionGenerator::visit(ast::Variable &variable) {
  return named_values->at(variable.name.lexeme);
}

void *ExpressionGenerator::visit(ast::StringLiteral &literal) {
  return builder->CreateGlobalStringPtr(StringRef(literal.value));
}
