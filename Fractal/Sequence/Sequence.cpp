// Sequence.cpp
// Contains Build System related definitions. Sequence is Fractal's build system
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "Sequence.h"
#include "Utilities.h"

#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include "Analysis/SemanticAnalyzer.h"
#include "CodeGeneration/CodeGenerator.h"
#include "CodeEmission/IntelCodeEmission.h"

namespace Fractal {
    void to_json(json& j, const Project& p) {
        j = json{ {"Name", p.name}, {"SourcePath", p.srcPath}, {"BuildPath", p.outPath}, {"Architecture", p.architecture} };
    }

    void from_json(const json& j, Project& p) {
        j.at("Name").get_to(p.name);
        j.at("SourcePath").get_to(p.srcPath);
        j.at("BuildPath").get_to(p.outPath);
        j.at("Architecture").get_to(p.architecture);
    }

    bool createProject(const std::filesystem::path& projectDir, const Project& project) {
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

    bool buildProject(const std::filesystem::path& projectDir) {
        if(!std::filesystem::exists(projectDir / "build_config.json")) {
            std::cout << "There is no build_config.json file in the current directory.\n";
            return false;
        }

        Project project{};
        json doc{json::parse(readFile(projectDir / "build_config.json"))};
        doc.get_to(project);

        Platform platform;
        if (project.architecture == "x86_64-intel-win") platform = Platform::Win;
        else if (project.architecture == "x86_64-intel-mac") platform = Platform::Mac;
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

        if(!parser.parse(lexer.getTokenList())) {
            errorHandler.outputErrors();
            return false;
        }

        for (auto& definition : parser.definitions())
            definition->print();
        for(auto& statement : parser.statements())
            statement->print();

        if (!semanticAnalyzer.analyze(&parser.program())) {
            errorHandler.outputWarnings();
            errorHandler.outputErrors();
            return false;
        }
        errorHandler.outputWarnings();

        std::cout << "Analysis Completed" << '\n';

        for (auto instruction : codeGenerator.generate(parser.program(), platform))
            instruction->print();

        std::cout << '\n';

        if(platform == Platform::Win) {
            std::cout << emitter.emit(&codeGenerator.instructions(), codeGenerator.externals(), Platform::Win);

            std::filesystem::path intermediate = projectDir / project.outPath / "intermediate";
            if(!std::filesystem::exists(intermediate))
                std::filesystem::create_directory(intermediate);
            writeFile(emitter.output(), intermediate / (project.name + ".asm"));

            std::string path = project.outPath + "\\intermediate\\" + project.name;

            system(("nasm -f elf64 " + path + ".asm -o " + path + ".o").c_str());
            system(("gcc " + path + ".o -o " + path + ".exe").c_str());
        }
        else if (platform == Platform::Mac) {
            std::cout << emitter.emit(&codeGenerator.instructions(), codeGenerator.externals(), Platform::Mac);

            std::filesystem::path intermediate = projectDir / project.outPath / "intermediate";
            if (!std::filesystem::exists(intermediate))
                std::filesystem::create_directory(intermediate);
            writeFile(emitter.output(), intermediate / (project.name + ".asm"));

            std::string path = project.outPath + "/intermediate/" + project.name;

            system(("nasm -f macho64 " + path + ".asm -o " + path + ".o").c_str());
            system(("arch -x86_64 gcc " + path + ".o -o " + path).c_str());
        }
        

        return true;
    }
}