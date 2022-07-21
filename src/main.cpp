#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#include "codegen.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

int main(int argc, char **argv) {
  auto release = false;
  auto dump = false;
  std::string output;
  std::vector<std::filesystem::path> source_inputs;

  // todo: more robust argument parsing
  //  - forbid --dump and --output both being specified, etc
  for (int i = 1; i < argc; ++i) {
    std::string argument(argv[i]);
    if (argument == "--dump") {
      dump = true;
    } else if (argument == "--release") {
      release = true;
    } else if (argument == "--output") {
      if (i + 1 == argc) {
        errs() << "Expected an output name";
        return 64;
      }

      i += 1;
      output = std::string(argv[i]);
    } else {
      source_inputs.emplace_back(argument);
    }
  }

  if (source_inputs.empty()) {
    errs() << "Expected source files";
    return 64; //
  }

  // This could be more specific, which would be faster
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();

  std::string error;
  auto target_triple = sys::getDefaultTargetTriple();
  auto target = TargetRegistry::lookupTarget(target_triple, error);
  if (!target) {
    errs() << error;
    return 1;
  }

  TargetOptions opt;
  auto cpu = "generic";
  auto features = "";
  auto relocation_model = Optional<Reloc::Model>();
  auto target_machine = target->createTargetMachine(
      target_triple, cpu, features, opt, relocation_model);

  std::ostringstream linker_command;
  linker_command << "ld";

  for (const auto &source_path : source_inputs) {
    std::ifstream file(source_path);
    file.ignore(std::numeric_limits<std::streamsize>::max());
    auto length = file.gcount();
    file.clear();
    file.seekg(0, std::ios_base::beg);

    std::vector<char> buffer(length);
    if (!file.read(buffer.data(), length))
      return 66;

    // todo: is EOF _really_ necessary?
    buffer.push_back((char)EOF);

    Lexer lexer{&buffer, 0};
    Parser parser(&lexer);

    auto program = parser.parse_program();

    CodeGen generator;
    auto module = generator.compile_module(source_path, program, release);

    if (dump) {
      module->print(outs(), nullptr);
      outs() << "\n";
      continue;
    }

    module->setDataLayout(target_machine->createDataLayout());
    module->setTargetTriple(target_triple);

    // Darwin only supports dwarf2.
    if (Triple(sys::getProcessTriple()).isOSDarwin())
      module->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);

    std::filesystem::path object_file_path(source_path);
    object_file_path.replace_extension("o");
    std::error_code error_code;
    raw_fd_ostream dest(object_file_path.string(), error_code,
                        sys::fs::OF_None);

    if (error_code) {
      errs() << "Could not open file: " << error_code.message();
      return 1;
    }

    legacy::PassManager pass;
    auto FileType = CGFT_ObjectFile;

    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
      errs() << "TheTargetMachine can't emit a file of this type";
      return 1;
    }

    pass.run(*module);
    dest.flush();

    linker_command << " " << object_file_path.string();
  }

  linker_command << " -o";
  linker_command << " " << (output.empty() ? "program" : output);
  linker_command << " -lSystem";
  linker_command << " -L$(xcode-select -p)/SDKs/MacOSX.sdk/usr/lib";

  if (dump) {
    std::cout << "Linker line: " << linker_command.str() << std::endl;
    return 0;
  }

  system(linker_command.str().c_str());

  return 0;
}
