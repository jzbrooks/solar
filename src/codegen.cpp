#include "codegen.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Host.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

#include <cassert>

using namespace llvm;
using namespace std;

static Type *llvm_type_for(const ast::TypeInfo &type_info,
                           LLVMContext &context) {
  switch (type_info.type) {
  case ast::TypeInfo::Type::BOOL:
    return Type::getInt1Ty(context);
  case ast::TypeInfo::Type::INTEGER:
    return type_info.size == 32 ? Type::getInt32Ty(context)
                                : Type::getInt64Ty(context);
  case ast::TypeInfo::Type::FLOAT: {
    return type_info.size == 32 ? Type::getFloatTy(context)
                                : Type::getDoubleTy(context);
    break;
  }
  default:
    assert(0);
    break;
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
  context = new LLVMContext;
  builder = new IRBuilder(*context);
  debug_info_generator = nullptr;
  named_values = new std::unordered_map<string, AllocaInst *>();
}

CodeGen::~CodeGen() {
  delete context;
  delete builder;
  delete debug_info_generator;
  delete named_values;
}

Module *CodeGen::compile_module(const filesystem::path &source_file,
                                ast::Program *program, bool release) {

  auto module = new Module(source_file.c_str(), *context);

  if (!release)
    debug_info_generator = new DebugInfoGenerator(module, builder, source_file);

  ExpressionGenerator expressionGenerator(module, builder, debug_info_generator,
                                          named_values);

  StatementGenerator statementGenerator(module, builder, debug_info_generator,
                                        expressionGenerator, named_values,
                                        release);

  // Add printf manually
  // todo(jzb): Add debug info for printf?
  //  really this should be in a standard library eventually
  std::vector<Type *> args = {Type::getInt8PtrTy(module->getContext())};
  auto printf_type =
      FunctionType::get(Type::getInt32Ty(module->getContext()),
                        ArrayRef<Type *>(args.data(), args.size()), true);

  auto attributes =
      AttributeList::get(module->getContext(), 1U, {Attribute::NoAlias});
  module->getOrInsertFunction("printf", printf_type, attributes);

  for (const auto &statement : program->statements) {
    statement->accept(statementGenerator);
  }

  if (debug_info_generator)
    debug_info_generator->finalize();

  return module;
}

StatementGenerator::StatementGenerator(
    Module *module, IRBuilder<> *builder,
    DebugInfoGenerator *debug_info_generator,
    ExpressionGenerator &expressionGenerator,
    std::unordered_map<std::string, AllocaInst *> *named_values, bool release)
    : module(module), builder(builder),
      debug_info_generator(debug_info_generator),
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
  const auto type = llvm_type_for(node.type_info, module->getContext());
  const auto function = builder->GetInsertBlock()->getParent();
  IRBuilder<> temp_builder(&function->getEntryBlock(),
                           function->getEntryBlock().begin());
  const auto alloca =
      create_entry_block_alloca(*builder, function, type, node.name.lexeme);
  named_values->insert({node.name.lexeme, alloca});

  if (debug_info_generator)
    debug_info_generator->attach_debug_info(node, alloca,
                                            function->getSubprogram());

  if (node.initializer) {
    const auto value =
        static_cast<Value *>(node.initializer->accept(expressionGenerator));
    builder->CreateStore(value, alloca);
  } else {
    assert(0); // todo: initializers are required for now?
  }
}

void StatementGenerator::visit(ast::ExpressionStatement &node) {
  node.expression->accept(expressionGenerator);
}

void StatementGenerator::visit(ast::Block &block) {
  if (debug_info_generator)
    debug_info_generator->emit_location(&block);

  for (const auto &statement : block.statements) {
    statement->accept(*this);
  }
}

void StatementGenerator::visit(ast::Function &function) {
  SmallVector<Type *, 8> argument_types;
  for (const auto &parameter : function.prototype.parameter_list) {
    argument_types.push_back(
        llvm_type_for(parameter.type_info, module->getContext()));
  }

  auto return_type =
      llvm_type_for(function.prototype.return_type_info, module->getContext());
  if (!return_type) {
    return_type = Type::getVoidTy(module->getContext());
  }

  auto type = FunctionType::get(return_type, argument_types, false);

  auto func = Function::Create(type, GlobalValue::LinkageTypes::ExternalLinkage,
                               function.prototype.name.lexeme, module);

  if (debug_info_generator) {
    debug_info_generator->attach_debug_info(function, func);
    debug_info_generator->lexical_scopes.push_back(func->getSubprogram());
  }

  // Unset the location for the prologue emission (leading instructions with no
  // location in a function are considered part of the prologue and the debugger
  // will run past them when breaking on a function)
  builder->SetCurrentDebugLocation(DebugLoc());

  assert(function.prototype.parameter_list.size() == func->arg_size());

  auto entry = BasicBlock::Create(module->getContext(), "entry", func);
  builder->SetInsertPoint(entry);

  named_values->clear();
  for (auto i = 0; i < function.prototype.parameter_list.size(); ++i) {
    const auto &parameter = function.prototype.parameter_list[i];
    const auto &arg = func->getArg(i);
    arg->setName(parameter.name.lexeme);

    const auto alloca = create_entry_block_alloca(
        *builder, func, arg->getType(), arg->getName());

    if (debug_info_generator)
      debug_info_generator->attach_debug_info(parameter, arg, alloca,
                                              func->getSubprogram());

    builder->CreateStore(arg, alloca);
    named_values->insert_or_assign(arg->getName().str(), alloca);
  }

  function.body->accept(*this);

  if (func->getBasicBlockList().back().getTerminator() == nullptr) {
    builder->CreateRetVoid();
  }

  if (debug_info_generator)
    debug_info_generator->lexical_scopes.pop_back();

  verifyFunction(*func);

  function_pass_manager->run(*func);
}

void StatementGenerator::visit(ast::Return &return_statement) {
  if (debug_info_generator)
    debug_info_generator->emit_location(&return_statement);

  auto value = static_cast<Value *>(
      return_statement.return_value->accept(expressionGenerator));
  builder->CreateRet(value);
}

ExpressionGenerator::ExpressionGenerator(
    Module *module, IRBuilder<> *builder,
    DebugInfoGenerator *debug_info_generator,
    unordered_map<string, AllocaInst *> *named_values)
    : module(module), builder(builder),
      debug_info_generator(debug_info_generator), named_values(named_values) {}

void *ExpressionGenerator::visit(ast::LiteralValueExpression &expression) {
  if (debug_info_generator)
    debug_info_generator->emit_location(&expression);

  const auto type = llvm_type_for(expression.type_info, module->getContext());
  const auto size = type->getScalarSizeInBits();

  switch (type->getTypeID()) {
  case Type::TypeID::IntegerTyID:
    return ConstantInt::get(type, size == 64 ? expression.value.int64
                                             : expression.value.int32);
  case Type::TypeID::FloatTyID:
    return ConstantFP::get(type, expression.value.float32);
  case Type::TypeID::DoubleTyID:
    return ConstantFP::get(type, expression.value.float64);
  default:
    assert(0);
  }

  return nullptr;
}

void *ExpressionGenerator::visit(ast::Binop &binop) {
  if (debug_info_generator)
    debug_info_generator->emit_location(&binop);

  const auto left = static_cast<Value *>(binop.left->accept(*this));
  const auto right = static_cast<Value *>(binop.right->accept(*this));
  const auto operation = binop.operation;

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
  if (debug_info_generator)
    debug_info_generator->emit_location(&condition);

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
  if (debug_info_generator)
    debug_info_generator->emit_location(&call);

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
  if (debug_info_generator)
    debug_info_generator->emit_location(&variable);

  auto value = named_values->at(variable.name.lexeme);
  assert(value);

  return builder->CreateLoad(value, variable.name.lexeme.c_str());
}

void *ExpressionGenerator::visit(ast::StringLiteral &literal) {
  if (debug_info_generator)
    debug_info_generator->emit_location(&literal);
  return builder->CreateGlobalStringPtr(StringRef(literal.value));
}

DebugInfoGenerator::DebugInfoGenerator(Module *module, IRBuilder<> *ir_builder,
                                       const std::filesystem::path &file)
    : debug_info_builder(new DIBuilder(*module)), ir_builder(ir_builder) {

  // Darwin only supports dwarf2.
  if (Triple(sys::getProcessTriple()).isOSDarwin())
    module->addModuleFlag(Module::Warning, "Dwarf Version", 2);

  // Add the current debug info version into the module.
  module->addModuleFlag(Module::Warning, "Debug Info Version",
                        DEBUG_METADATA_VERSION);

  auto di_file = debug_info_builder->createFile(file.filename().c_str(),
                                                file.parent_path().c_str());

  // Debug information is never generated for release mode
  compile_unit = debug_info_builder->createCompileUnit(
      dwarf::DW_LANG_C, di_file, "Solar Compiler", false, "", 0);
}

DebugInfoGenerator::~DebugInfoGenerator() { delete debug_info_builder; }

void DebugInfoGenerator::emit_location(const ast::Node *node) {
  assert(node);

  auto scope = lexical_scopes.empty() ? compile_unit : lexical_scopes.back();
  auto location = DILocation::get(scope->getContext(), node->position.line,
                                  node->position.column, scope);

  ir_builder->SetCurrentDebugLocation(location);
}

void DebugInfoGenerator::attach_debug_info(const ast::Function &ast_function,
                                           Function *llvm_function) {
  std::vector<Metadata *> func_metadata;
  func_metadata.push_back(get_type(ast_function.prototype.return_type_info));

  for (const auto &arg : ast_function.prototype.parameter_list) {
    func_metadata.push_back(get_type(arg.type_info));
  }

  auto parameter_types =
      debug_info_builder->getOrCreateTypeArray(func_metadata);
  auto subroutine_type =
      debug_info_builder->createSubroutineType(parameter_types);

  auto file = compile_unit->getFile();

  auto subprogram = debug_info_builder->createFunction(
      file, ast_function.prototype.name.lexeme, StringRef(), file,
      ast_function.position.line, subroutine_type, 0, DINode::FlagPrototyped,
      DISubprogram::SPFlagDefinition);

  llvm_function->setSubprogram(subprogram);
}

void DebugInfoGenerator::attach_debug_info(const ast::Parameter &parameter,
                                           Argument *arg, AllocaInst *alloca,
                                           DISubprogram *subprogram) {

  auto arg_debug_info = debug_info_builder->createParameterVariable(
      subprogram, arg->getName(), arg->getArgNo(), subprogram->getFile(),
      parameter.position.line, get_type(parameter.type_info), true);

  debug_info_builder->insertDeclare(
      alloca, arg_debug_info, debug_info_builder->createExpression(),
      DILocation::get(subprogram->getContext(), parameter.position.line, 0,
                      subprogram),
      ir_builder->GetInsertBlock());
}

void DebugInfoGenerator::attach_debug_info(const ast::VariableDeclaration &decl,
                                           AllocaInst *alloca,
                                           DISubprogram *subprogram) {

  auto variable_debug_info = debug_info_builder->createAutoVariable(
      subprogram->getScope(), decl.name.lexeme, subprogram->getFile(),
      decl.position.line, get_type(decl.type_info), true);

  auto location = DILocation::get(subprogram->getContext(), decl.position.line,
                                  decl.position.column, subprogram);

  debug_info_builder->insertDeclare(alloca, variable_debug_info,
                                    debug_info_builder->createExpression(),
                                    location, ir_builder->GetInsertBlock());
}

DIBasicType *DebugInfoGenerator::get_type(const ast::TypeInfo &type_info) {
  // todo: maybe cache these?
  switch (type_info.type) {
  case ast::TypeInfo::Type::BOOL:
    return debug_info_builder->createBasicType("bool", 1,
                                               dwarf::DW_ATE_boolean);
    break;
  case ast::TypeInfo::Type::INTEGER: {
    if (type_info.is_signed) {
      auto name = type_info.size == 32 ? "i32" : "i64";
      return debug_info_builder->createBasicType(name, type_info.size,
                                                 dwarf::DW_ATE_signed);
    } else {
      auto name = type_info.size == 32 ? "u32" : "u64";
      return debug_info_builder->createBasicType(name, type_info.size,
                                                 dwarf::DW_ATE_unsigned);
    }
    break;
  }
  case ast::TypeInfo::Type::FLOAT: {
    auto name = type_info.size == 32 ? "f32" : "f64";
    return debug_info_builder->createBasicType(name, type_info.size,
                                               dwarf::DW_ATE_float);
    break;
  }
  default:
    assert(0);
    break;
  }

  assert(0); // todo: user defined types
  return nullptr;
}

void DebugInfoGenerator::finalize() const { debug_info_builder->finalize(); }