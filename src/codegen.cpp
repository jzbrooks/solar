#include "codegen.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

#include <cassert>

using namespace llvm;
using namespace std;

static Type *llvm_type_for(const Token &type_token,
                           llvm::LLVMContext &context) {
  if (type_token == ast::Type::Primitive::INT64 ||
      type_token == ast::Type::Primitive::UINT64) {
    return Type::getInt64Ty(context);
  } else if (type_token == ast::Type::Primitive::INT32 ||
             type_token == ast::Type::Primitive::UINT32) {
    return Type::getInt32Ty(context);
  } else if (type_token == ast::Type::Primitive::FLOAT64) {
    return Type::getDoubleTy(context);
  } else if (type_token == ast::Type::Primitive::FLOAT32) {
    return Type::getFloatTy(context);
  } else if (type_token == ast::Type::Primitive::BOOL) {
    return Type::getInt1Ty(context);
  }

  assert(0); // todo: user defined types
  return nullptr;
}

static AllocaInst *create_entry_block_alloca(IRBuilder<> &builder,
                                             Function *function, Type *type,
                                             const Twine &name) {
  IRBuilder<> temp_builder(&function->getEntryBlock(),
                           function->getEntryBlock().begin());
  return temp_builder.CreateAlloca(type, nullptr, name);
}

CodeGen::CodeGen() {
  context = new LLVMContext();
  builder = new IRBuilder(*context);
  debug_info_builder = nullptr;
  named_values = new std::unordered_map<string, AllocaInst *>();
}

CodeGen::~CodeGen() {
  delete context;
  delete builder;
  delete debug_info_builder;
  delete named_values;
}

Module *CodeGen::compile_module(const char *id, ast::Program *program, bool release) {
  auto module = new Module(id, *context);
  debug_info_builder = new DIBuilder(*module);

  ExpressionGenerator expressionGenerator(module, builder, debug_info_builder, named_values);
  StatementGenerator statementGenerator(module, builder, debug_info_builder, expressionGenerator,
                                        named_values, release);

  // todo: this file creation might not be correct
  debug_info_builder->createCompileUnit(
      dwarf::DW_LANG_C, debug_info_builder->createFile(id, "."),
      "Solar Compiler", release, "", 0);

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

  debug_info_builder->finalize();

  return module;
}

StatementGenerator::StatementGenerator(
    Module *module, llvm::IRBuilder<> *builder,
    DIBuilder *debug_info_builder,
    ExpressionGenerator &expressionGenerator,
    std::unordered_map<std::string, llvm::AllocaInst *> *named_values,
    bool release)
    : module(module), builder(builder), debug_info_builder(debug_info_builder),
      expressionGenerator(expressionGenerator), named_values(named_values),
      function_pass_manager(new legacy::FunctionPassManager(module)) {

  if (release) {
    // Convert alloca instructions to registers
    function_pass_manager->add(createPromoteMemoryToRegisterPass());
    function_pass_manager->add(createGVNPass());
    function_pass_manager->add(createReassociatePass());
    function_pass_manager->add(createCFGSimplificationPass());
    function_pass_manager->add(createDeadCodeEliminationPass());
    function_pass_manager->add(createInstructionCombiningPass());
  }

  function_pass_manager->doInitialization();
}

void StatementGenerator::visit(ast::VariableDeclaration &node) {
  auto type = llvm_type_for(node.type, module->getContext());
  auto function = builder->GetInsertBlock()->getParent();
  IRBuilder<> temp_builder(&function->getEntryBlock(),
                           function->getEntryBlock().begin());
  auto alloca =
      create_entry_block_alloca(*builder, function, type, node.name.lexeme);
  named_values->insert({node.name.lexeme, alloca});

  if (node.initializer) {
    auto value = (Value *)node.initializer->accept(expressionGenerator);
    builder->CreateStore(value, alloca);
  } else {
    assert(0); // todo: initializers are required for now?
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
    argument_types.push_back(
        llvm_type_for(parameter.type, module->getContext()));
  }

  auto return_type =
      llvm_type_for(function.prototype->return_type, module->getContext());
  if (!return_type) {
    return_type = Type::getVoidTy(module->getContext());
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

  named_values->clear();
  for (auto &arg : func->args()) {
    auto alloca =
        create_entry_block_alloca(*builder, func, arg.getType(), arg.getName());
    builder->CreateStore(&arg, alloca);
    named_values->insert_or_assign(arg.getName().str(), alloca);
  }

  function.body->accept(*this);

  if (func->getBasicBlockList().back().getTerminator() == nullptr) {
    builder->CreateRetVoid();
  }

  //#ifdef DEBUG
  verifyFunction(*func);
  //#endif

  function_pass_manager->run(*func);
}

void StatementGenerator::visit(ast::Return &return_statement) {
  auto value =
      (Value *)return_statement.return_value->accept(expressionGenerator);
  builder->CreateRet(value);
}

ExpressionGenerator::ExpressionGenerator(
    llvm::Module *module, llvm::IRBuilder<> *builder,
    DIBuilder *debug_info_builder,
    unordered_map<string, llvm::AllocaInst *> *named_values)
    : module(module), builder(builder), debug_info_builder(debug_info_builder), named_values(named_values) {}

void *ExpressionGenerator::visit(ast::LiteralValueExpression &expression) {
  auto type = llvm_type_for(expression.type.name, module->getContext());
  auto size = type->getScalarSizeInBits();

  switch (type->getTypeID()) {
  case llvm::Type::TypeID::IntegerTyID:
    return ConstantInt::get(type, size == 64 ? expression.value.int64
                                             : expression.value.int32);
  case llvm::Type::TypeID::FloatTyID:
    return ConstantFP::get(type, expression.value.float32);
  case llvm::Type::TypeID::DoubleTyID:
    return ConstantFP::get(type, expression.value.float64);
  default:
    assert(0);
  }

  return nullptr;
}

void *ExpressionGenerator::visit(ast::Binop &binop) {
  auto left = (Value *)binop.left->accept(*this);
  auto right = (Value *)binop.right->accept(*this);
  auto operation = binop.operation;

  switch (operation) {
  case ast::Operation::ADD: {

    return left->getType()->isFloatingPointTy()
               ? builder->CreateFAdd(left, right)
               : builder->CreateAdd(left, right);
  }
  case ast::Operation::SUBTRACT: {
    return left->getType()->isFloatingPointTy()
               ? builder->CreateFSub(left, right)
               : builder->CreateSub(left, right);
  }
  case ast::Operation::MULTIPLY: {
    return left->getType()->isFloatingPointTy()
               ? builder->CreateFMul(left, right)
               : builder->CreateMul(left, right);
  }
  case ast::Operation::DIVIDE: {
    auto isLeftFloat = left->getType()->isFloatingPointTy();
    auto isRightFloat = right->getType()->isFloatingPointTy();

    return isLeftFloat || isRightFloat ? builder->CreateFDiv(left, right)
                                       : builder->CreateSDiv(left, right);
  }
  case ast::Operation::COMPARE_IS_EQUAL: {
    auto predicate = left->getType()->isFloatingPointTy()
                         ? CmpInst::Predicate::FCMP_OEQ
                         : CmpInst::Predicate::ICMP_EQ;

    return builder->CreateCmp(predicate, left, right);
  }
  case ast::Operation::COMPARE_IS_NOT_EQUAL: {
    auto predicate = left->getType()->isFloatingPointTy()
                         ? CmpInst::Predicate::FCMP_ONE
                         : CmpInst::Predicate::ICMP_NE;

    return builder->CreateCmp(predicate, left, right);
  }
  case ast::Operation::COMPARE_IS_LESS: {
    auto predicate = left->getType()->isFloatingPointTy()
                         ? CmpInst::Predicate::FCMP_OLT
                         : CmpInst::Predicate::ICMP_SLT;

    return builder->CreateCmp(predicate, left, right);
  }
  case ast::Operation::COMPARE_IS_GREATER: {
    auto predicate = left->getType()->isFloatingPointTy()
                         ? CmpInst::Predicate::FCMP_OGT
                         : CmpInst::Predicate::ICMP_SGT;

    return builder->CreateCmp(predicate, left, right);
  }
  case ast::Operation::COMPARE_IS_LESS_OR_EQUAL: {
    auto predicate = left->getType()->isFloatingPointTy()
                         ? CmpInst::Predicate::FCMP_OLE
                         : CmpInst::Predicate::ICMP_SLE;

    return builder->CreateCmp(predicate, left, right);
  }
  case ast::Operation::COMPARE_IS_GREATER_OR_EQUAL: {
    auto predicate = left->getType()->isFloatingPointTy()
                         ? CmpInst::Predicate::FCMP_OGE
                         : CmpInst::Predicate::ICMP_SGE;

    return builder->CreateCmp(predicate, left, right);
  }
  default:
    assert(0); // todo: codegen errors
  }
}

void *ExpressionGenerator::visit(ast::Condition &condition) {
  auto if_condition = static_cast<Value *>(condition.condition->accept(*this));
  assert(if_condition);

  if_condition->setName("if_condition");

  auto function = builder->GetInsertBlock()->getParent();

  auto then_block = BasicBlock::Create(module->getContext(), "then", function);
  auto otherwise_block = BasicBlock::Create(module->getContext(), "else");
  auto merge_block = BasicBlock::Create(module->getContext(), "merge");

  builder->CreateCondBr(if_condition, then_block, otherwise_block);
  builder->SetInsertPoint(then_block);

  auto then_value = static_cast<Value *>(condition.then->accept(*this));
  assert(then_value);

  builder->CreateBr(merge_block);

  // condition.then->accept(*this); can change the current block,
  // so make sure we're tracking the previous
  then_block = builder->GetInsertBlock();

  function->getBasicBlockList().push_back(otherwise_block);
  builder->SetInsertPoint(otherwise_block);

  auto otherwise_value =
      static_cast<Value *>(condition.otherwise->accept(*this));
  assert(otherwise_value);

  builder->CreateBr(merge_block);
  otherwise_block = builder->GetInsertBlock();

  function->getBasicBlockList().push_back(merge_block);
  builder->SetInsertPoint(merge_block);

  // Then and else must be the same type
  auto phi = builder->CreatePHI(then_value->getType(), 2, "if_expr_tmp");

  phi->addIncoming(then_value, then_block);
  phi->addIncoming(otherwise_value, otherwise_block);

  return phi;
}

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
  auto value = named_values->at(variable.name.lexeme);
  assert(value);

  return builder->CreateLoad(value, variable.name.lexeme.c_str());
}

void *ExpressionGenerator::visit(ast::StringLiteral &literal) {
  return builder->CreateGlobalStringPtr(StringRef(literal.value));
}
