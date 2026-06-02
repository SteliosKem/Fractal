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

std::string defaultArchitecture() {
#if defined(_WIN32) || defined(_WIN64)
    return "x86_64-intel-win";
#else
    return "x86_64-intel-mac";
#endif
}

bool createProject(const std::filesystem::path &projectDir, const Project &project) {
    writeFile(json(project).dump(), projectDir / "build_config.json");

    std::string sampleCode = R"(/* Sample Fractal main file.
This file, which has the same name as the project, acts as the main function of the program.
Program execution starts from the first statement outside of the definitions header. */

<define>

fn main(): i32 {
    return 0;
}

<!define>
)";

    std::filesystem::path srcDir = projectDir / project.srcPath;
    std::filesystem::path outDir = projectDir / project.outPath;

    std::filesystem::create_directory(srcDir);
    std::filesystem::create_directory(outDir);

    writeFile(sampleCode, srcDir / (project.name + ".frc"));

    return true;
}

// Parses an architecture string to a Platform. Returns false on unknown values.
static bool parseArchitecture(const std::string &arch, Platform &platform) {
    if (arch == "x86_64-intel-win") { platform = Platform::Win; return true; }
    if (arch == "x86_64-intel-mac") { platform = Platform::Mac; return true; }
    return false;
}

// Runs the compiler pipeline on a single source file and writes <stem>.asm,
// <stem>.o, and the final native executable into outDir. The stem is the
// source file's name without extension.
static bool compilePipeline(const std::filesystem::path &sourceFile,
                            const std::filesystem::path &outDir,
                            Platform platform,
                            const BuildOptions &options) {
    ErrorHandler errorHandler;
    Lexer lexer(&errorHandler);
    Parser parser(&errorHandler);
    SemanticAnalyzer semanticAnalyzer(&errorHandler);
    CodeGenerator codeGenerator(&errorHandler);
    IntelCodeEmission emitter{};

    if (!lexer.analyze(sourceFile)) {
        errorHandler.outputErrors();
        return false;
    }
    if (options.verbose) lexer.print();

    if (!parser.parse(lexer.getTokenList())) {
        errorHandler.outputErrors();
        return false;
    }
    if (options.verbose) {
        for (auto &definition : parser.definitions()) definition->print();
        for (auto &statement : parser.statements()) statement->print();
    }

    if (!semanticAnalyzer.analyze(&parser.program())) {
        errorHandler.outputWarnings();
        errorHandler.outputErrors();
        return false;
    }
    errorHandler.outputWarnings();

    const auto &instructions = codeGenerator.generate(parser.program(), platform);
    if (options.verbose) {
        std::cout << "Analysis Completed\n";
        for (auto instruction : instructions) instruction->print();
        std::cout << '\n';
    }

    std::string asmOutput = emitter.emit(&codeGenerator.instructions(),
                                         codeGenerator.externals(), platform);
    if (options.verbose) std::cout << asmOutput;

    std::filesystem::create_directories(outDir);

    std::string stem = sourceFile.stem().string();
    std::filesystem::path asmPath = outDir / (stem + ".asm");
    std::filesystem::path objPath = outDir / (stem + ".o");

    writeFile(asmOutput, asmPath);

    bool ok = false;
    if (platform == Platform::Win) {
        std::filesystem::path exePath = outDir / (stem + ".exe");
        ok = runCommand("nasm -f win64 " + quote(asmPath) + " -o " + quote(objPath))
          && runCommand("gcc " + quote(objPath) + " -o " + quote(exePath));
    } else {
        std::filesystem::path binPath = outDir / stem;
        ok = runCommand("nasm -f macho64 " + quote(asmPath) + " -o " + quote(objPath))
          && runCommand("arch -x86_64 gcc " + quote(objPath) + " -o " + quote(binPath));
    }

    if (!options.keepAsm) {
        std::error_code ec;
        std::filesystem::remove(asmPath, ec);
    }

    return ok;
}

bool buildProject(const std::filesystem::path &projectDir, const BuildOptions &options) {
    if (!std::filesystem::exists(projectDir / "build_config.json")) {
        std::cout << "There is no build_config.json file in the current directory.\n";
        return false;
    }

    Project project{};
    json doc = json::parse(readFile(projectDir / "build_config.json"));
    doc.get_to(project);

    Platform platform;
    if (!parseArchitecture(project.architecture, platform)) {
        std::cout << "Invalid architecture '" << project.architecture
                  << "' specified in build config. Aborting.\n";
        return false;
    }

    std::filesystem::path sourceFile =
        projectDir / project.srcPath / (project.name + ".frc");
    std::filesystem::path outDir = projectDir / project.outPath / "intermediate";

    return compilePipeline(sourceFile, outDir, platform, options);
}

bool buildSingleFile(const std::filesystem::path &sourceFile,
                     const std::filesystem::path &outDir,
                     const BuildOptions &options) {
    if (!std::filesystem::exists(sourceFile)) {
        std::cout << "Source file does not exist: " << sourceFile << '\n';
        return false;
    }

    Platform platform;
    parseArchitecture(defaultArchitecture(), platform);

    std::filesystem::path target = outDir.empty()
        ? sourceFile.parent_path()
        : outDir;

    return compilePipeline(sourceFile, target, platform, options);
}

} // namespace Fractal
