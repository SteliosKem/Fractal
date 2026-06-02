// Sequence.cpp
// Contains Build System related definitions. Sequence is Fractal's build system
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "Sequence.h"
#include "Utilities.h"

#include "Analysis/SemanticAnalyzer.h"
#include "CodeEmission/IntelCodeEmission.h"
#include "CodeGeneration/CodeGenerator.h"
#include "Lexer/Lexer.h"
#include "Parser/Parser.h"

#include <cstdlib>
#include <iostream>

namespace Fractal {
static std::string quote(const std::filesystem::path &p) {
  return "\"" + p.string() + "\"";
}

static bool runCommand(const std::string &cmd) {
  int rc = std::system(cmd.c_str());
  if (rc != 0) {
    std::cout << "Command failed (exit " << rc << "): " << cmd << '\n';
    return false;
  }
  return true;
}

void to_json(json &j, const Project &p) {
  j = json{{"Name", p.name},
           {"SourcePath", p.srcPath},
           {"BuildPath", p.outPath},
           {"Architecture", p.architecture}};
}

void from_json(const json &j, Project &p) {
  j.at("Name").get_to(p.name);
  j.at("SourcePath").get_to(p.srcPath);
  j.at("BuildPath").get_to(p.outPath);
  j.at("Architecture").get_to(p.architecture);
}

bool createProject(const std::filesystem::path &projectDir,
                   const Project &project) {
  writeFile(json(project).dump(), projectDir / "build_config.json");

  std::string sampleCode = R"(/* Sample Fractal main file. 
This file, which has the same name as the project, acts as the main function of the program.
Program execution starts from the first statement outside of the definitions header. */

<define>

fn sampleFunction(): i32 {
    return 0;
}

<!define>

sampleFunction();)";

  std::filesystem::path srcDir = projectDir / project.srcPath;
  std::filesystem::path outDir = projectDir / project.outPath;

  std::filesystem::create_directory(srcDir);
  std::filesystem::create_directory(outDir);

  writeFile(sampleCode, srcDir / (project.name + ".frc"));

  return true;
}

bool buildProject(const std::filesystem::path &projectDir) {
  if (!std::filesystem::exists(projectDir / "build_config.json")) {
    std::cout
        << "There is no build_config.json file in the current directory.\n";
    return false;
  }

  Project project{};
  json doc = json::parse(readFile(projectDir / "build_config.json"));
  doc.get_to(project);

  Platform platform;
  if (project.architecture == "x86_64-intel-win")
    platform = Platform::Win;
  else if (project.architecture == "x86_64-intel-mac")
    platform = Platform::Mac;
  else {
    std::cout << "Invalid architecture specified in build config. Aborting.";
    return false;
  }

  std::filesystem::path srcPath = projectDir / project.srcPath;

  Fractal::ErrorHandler errorHandler;
  Fractal::Lexer lexer(&errorHandler);
  Fractal::Parser parser(&errorHandler);
  Fractal::SemanticAnalyzer semanticAnalyzer(&errorHandler);
  Fractal::CodeGenerator codeGenerator(&errorHandler);
  Fractal::IntelCodeEmission emitter{};

  if (!lexer.analyze(srcPath / (project.name + ".frc"))) {
    errorHandler.outputErrors();
    return false;
  }
  lexer.print();

  if (!parser.parse(lexer.getTokenList())) {
    errorHandler.outputErrors();
    return false;
  }

  for (auto &definition : parser.definitions())
    definition->print();
  for (auto &statement : parser.statements())
    statement->print();

  if (!semanticAnalyzer.analyze(&parser.program())) {
    errorHandler.outputWarnings();
    errorHandler.outputErrors();
    return false;
  }
  errorHandler.outputWarnings();

  std::cout << "Analysis Completed" << '\n';

  for (auto &definition : parser.definitions())
    definition->print();
  for (auto &statement : parser.statements())
    statement->print();

  for (auto instruction : codeGenerator.generate(parser.program(), platform))
    instruction->print();

  std::cout << '\n';

  std::filesystem::path intermediate =
      projectDir / project.outPath / "intermediate";
  if (!std::filesystem::exists(intermediate))
    std::filesystem::create_directory(intermediate);

  std::filesystem::path asmPath = intermediate / (project.name + ".asm");
  std::filesystem::path objPath = intermediate / (project.name + ".o");

  std::cout << emitter.emit(&codeGenerator.instructions(),
                            codeGenerator.externals(), platform);
  writeFile(emitter.output(), asmPath);

  if (platform == Platform::Win) {
    std::filesystem::path exePath = intermediate / (project.name + ".exe");
    if (!runCommand("nasm -f win64 " + quote(asmPath) + " -o " + quote(objPath)))
      return false;
    if (!runCommand("gcc " + quote(objPath) + " -o " + quote(exePath)))
      return false;
  } else if (platform == Platform::Mac) {
    std::filesystem::path binPath = intermediate / project.name;
    if (!runCommand("nasm -f macho64 " + quote(asmPath) + " -o " +
                    quote(objPath)))
      return false;
    if (!runCommand("arch -x86_64 gcc " + quote(objPath) + " -o " +
                    quote(binPath)))
      return false;
  }

  return true;
}
} // namespace Fractal