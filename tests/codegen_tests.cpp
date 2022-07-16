#include "catch/catch.hpp"

#include "../src/codegen.hpp"
#include "../src/lexer.hpp"
#include "../src/parser.hpp"

using namespace ast;
using namespace llvm;

static ast::Program *parse_program(std::string source) {
  source += (char)EOF;
  std::vector<char> input(source.begin(), source.end());
  Lexer lexer{&input, 0};
  Parser parser(&lexer);

  return parser.parse_program();
}

TEST_CASE("add_two function is generated", "[codegen]") {
  auto program = parse_program("func add_two(n: i32) -> i32 { return n + 2 }");

  CodeGen codegen;
  auto module = codegen.compile_module("test_module", program);
  const auto &function = module->getFunction("add_two");
  const auto &function_body = function->getEntryBlock();

  REQUIRE(function->getReturnType() ==
          llvm::Type::getInt32Ty(module->getContext()));

  auto &instruction = function_body.back();
  REQUIRE(isa<ReturnInst>(instruction));
  REQUIRE(std::any_of(function_body.begin(), function_body.end(),
                      [](const Instruction &instruction) {
                        return isa<BinaryOperator>(instruction) &&
                               instruction.getOpcode() == Instruction::Add;
                      }));
}

TEST_CASE("local_vars function is generated", "[codegen]") {
  auto program = parse_program("func local_vars(n: i32) -> i32 {\n"
                               "var a: i32 = 1\n"
                               "return a + n\n"
                               "}");

  CodeGen codegen;
  auto module = codegen.compile_module("test_module", program);
  const auto &function = module->getFunction("local_vars");
  const auto &function_body = function->getEntryBlock();

  auto instruction = function_body.rbegin();
  REQUIRE(isa<ReturnInst>(*instruction));
  REQUIRE(std::any_of(function_body.begin(), function_body.end(),
                      [](const Instruction &instruction) {
                        return isa<AllocaInst>(instruction);
                      }));
}

TEST_CASE("comparison greater than", "[codegen]") {
  auto program = parse_program("func greater_than(n: i64) -> i32 {"
                               "var a: bool = n > 3"
                               "return a"
                               "}");

  CodeGen codegen;
  auto module = codegen.compile_module("test_module", program);
  const auto &function = module->getFunction("greater_than");
  const auto &function_body = function->getEntryBlock();

  auto instruction = function_body.rbegin();
  REQUIRE(isa<ReturnInst>(*instruction));

  auto compare = std::find_if(
      function_body.begin(), function_body.end(),
      [](const Instruction &instruction) { return isa<CmpInst>(instruction); });

  REQUIRE(compare != function_body.end());

  auto compare_instruction = dyn_cast<CmpInst>(&(*compare));
  REQUIRE(compare_instruction->getPredicate() == llvm::CmpInst::ICMP_SGT);
}