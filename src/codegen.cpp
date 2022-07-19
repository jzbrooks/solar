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

static DIBasicType *dwarf_type_for(const Token &type_token,
                                   DIBuilder *builder) {
  if (type_token == ast::Type::Primitive::INT64) {
    return builder->createBasicType("i64", 64, dwarf::DW_ATE_signed);
  } else if (type_token == ast::Type::Primitive::UINT64) {
    return builder->createBasicType("u64", 64, dwarf::DW_ATE_unsigned);
  } else if (type_token == ast::Type::Primitive::INT32) {
    return builder->createBasicType("i32", 32, dwarf::DW_ATE_signed);
  } else if (type_token == ast::Type::Primitive::UINT32) {
    return builder->createBasicType("u32", 32, dwarf::DW_ATE_unsigned);
  } else if (type_token == ast::Type::Primitive::FLOAT64) {
    return builder->createBasicType("f64", 64, dwarf::DW_ATE_float);
  } else if (type_token == ast::Type::Primitive::FLOAT32) {
    return builder->createBasicType("f32", 32, dwarf::DW_ATE_float);
  } else if (type_token == ast::Type::Primitive::BOOL) {
    return builder->createBasicType("bool", 1, dwarf::DW_ATE_boolean);
  }

  assert(0); // todo: user defined types
  return nullptr;
}

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

void DebugInfo::emit_location(llvm::IRBuilder<> *ir_builder,
                              const ast::Node *node) {
  if (!node) {
    ir_builder->SetCurrentDebugLocation(DebugLoc());
    return;
  }

  auto scope = lexical_scopes.empty() ? compile_unit : lexical_scopes.back();
  auto location = DILocation::get(scope->getContext(), node->position.line,
                                  node->position.column, scope);

  ir_builder->SetCurrentDebugLocation(location);
}

CodeGen::CodeGen() {
  context = new LLVMContext;
  builder = new IRBuilder(*context);
  debug_info_builder = nullptr;
  named_values = new std::unordered_map<string, AllocaInst *>();
  debug_info = new DebugInfo;
}

CodeGen::~CodeGen() {
  delete context;
  delete builder;
  delete debug_info_builder;
  delete debug_info;
  delete named_values;
}

Module *CodeGen::compile_module(const char *id, ast::Program *program,
                                bool release) {
  auto module = new Module(id, *context);
  debug_info_builder = new DIBuilder(*module);

  ExpressionGenerator expressionGenerator(module, builder, debug_info_builder,
                                          debug_info, named_values);
  StatementGenerator statementGenerator(module, builder, debug_info_builder,
                                        debug_info, expressionGenerator,
                                        named_values, release);

  // todo: this file creation might not be correct
  debug_info->compile_unit = debug_info_builder->createCompileUnit(
      dwarf::DW_LANG_C, debug_info_builder->createFile(id, "."),
      "Solar Compiler", release, "", 0);

  // Add printf manually
  // todo(jzb): Add debug info for printf?
  //  really this should be in a standard library eventually
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
    Module *module, llvm::IRBuilder<> *builder, DIBuilder *debug_info_builder,
    DebugInfo *debug_info, ExpressionGenerator &expressionGenerator,
    std::unordered_map<std::string, llvm::AllocaInst *> *named_values,
    bool release)
    : module(module), builder(builder), debug_info_builder(debug_info_builder),
      debug_info(debug_info), expressionGenerator(expressionGenerator),
      named_values(named_values),
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

  auto subprogram = function->getSubprogram();
  auto file = subprogram->getFile();
  auto variable_debug_info = debug_info_builder->createAutoVariable(
      subprogram->getScope(), node.name.lexeme, file, node.position.line,
      dwarf_type_for(node.type, debug_info_builder), true);
  debug_info_builder->insertDeclare(
      alloca, variable_debug_info, debug_info_builder->createExpression(),
      DILocation::get(subprogram->getContext(), node.position.line, 0,
                      subprogram),
      builder->GetInsertBlock());

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
  debug_info->emit_location(builder, &block);

  for (const auto &statement : block.statements) {
    statement->accept(*this);
  }
}

void StatementGenerator::visit(ast::Function &function) {

  auto unit =
      debug_info_builder->createFile(debug_info->compile_unit->getFilename(),
                                     debug_info->compile_unit->getDirectory());

  SmallVector<Metadata *, 8> func_metadata;
  func_metadata.push_back(
      dwarf_type_for(function.prototype->return_type, debug_info_builder));
  for (const auto &arg : function.prototype->parameter_list) {
    func_metadata.push_back(dwarf_type_for(arg.type, debug_info_builder));
  }

  auto parameter_types =
      debug_info_builder->getOrCreateTypeArray(func_metadata);
  auto subroutine_type =
      debug_info_builder->createSubroutineType(parameter_types);

  auto subprogram = debug_info_builder->createFunction(
      unit, function.prototype->name.lexeme, StringRef(), unit,
      function.prototype->name.position.line, subroutine_type, 0);

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

  func->setSubprogram(subprogram);

  debug_info->lexical_scopes.push_back(subprogram);

  // Unset the location for the prologue emission (leading instructions with no
  // location in a function are considered part of the prologue and the debugger
  // will run past them when breaking on a function)
  debug_info->emit_location(builder, nullptr);

  assert(function.prototype->parameter_list.size() == func->arg_size());

  auto entry = BasicBlock::Create(module->getContext(), "entry", func);
  builder->SetInsertPoint(entry);

  named_values->clear();
  for (auto i = 0; i < function.prototype->parameter_list.size(); ++i) {
    const auto &parameter = function.prototype->parameter_list[i];
    const auto &arg = func->getArg(i);
    arg->setName(parameter.name.lexeme);

    auto alloca = create_entry_block_alloca(*builder, func, arg->getType(),
                                            arg->getName());

    auto arg_debug_info = debug_info_builder->createParameterVariable(
        subprogram, arg->getName(), arg->getArgNo(), unit,
        parameter.position.line,
        dwarf_type_for(parameter.type, debug_info_builder), true);

    debug_info_builder->insertDeclare(
        alloca, arg_debug_info, debug_info_builder->createExpression(),
        DILocation::get(subprogram->getContext(), parameter.position.line, 0,
                        subprogram),
        builder->GetInsertBlock());

    builder->CreateStore(arg, alloca);
    named_values->insert_or_assign(arg->getName().str(), alloca);
  }

  function.body->accept(*this);

  if (func->getBasicBlockList().back().getTerminator() == nullptr) {
    builder->CreateRetVoid();
  }

  debug_info->lexical_scopes.pop_back();

  verifyFunction(*func);

  function_pass_manager->run(*func);
}

void StatementGenerator::visit(ast::Return &return_statement) {
  debug_info->emit_location(builder, &return_statement);

  auto value =
      (Value *)return_statement.return_value->accept(expressionGenerator);
  builder->CreateRet(value);
}

ExpressionGenerator::ExpressionGenerator(
    llvm::Module *module, llvm::IRBuilder<> *builder,
    DIBuilder *debug_info_builder, DebugInfo *debug_info,
    unordered_map<string, llvm::AllocaInst *> *named_values)
    : module(module), builder(builder), debug_info_builder(debug_info_builder),
      debug_info(debug_info), named_values(named_values) {}

void *ExpressionGenerator::visit(ast::LiteralValueExpression &expression) {
  debug_info->emit_location(builder, &expression);

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
  debug_info->emit_location(builder, &binop);

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
  debug_info->emit_location(builder, &condition);

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
  debug_info->emit_location(builder, &call);

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
  debug_info->emit_location(builder, &variable);

  auto value = named_values->at(variable.name.lexeme);
  assert(value);

  return builder->CreateLoad(value, variable.name.lexeme.c_str());
}

void *ExpressionGenerator::visit(ast::StringLiteral &literal) {
  debug_info->emit_location(builder, &literal);
  return builder->CreateGlobalStringPtr(StringRef(literal.value));
}
