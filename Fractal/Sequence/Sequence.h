// Sequence.h
// Contains Build System related declarations. Sequence is Fractal's build system
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include "Common.h"
#include "json.hpp"

using json = nlohmann::json;

namespace Fractal {
    class Project {
    public:
        std::string name;
        std::string srcPath;
        std::string outPath;
        std::string architecture;
    };

    void to_json(json& j, const Project& p);
    void from_json(const json& j, Project& p);

    struct BuildOptions {
        bool verbose{ false };   // dump tokens, AST, IR, and asm to stdout
        bool keepAsm{ true };    // keep <name>.asm in intermediate/
    };

    // Returns a default architecture string for the host machine.
    std::string defaultArchitecture();

    bool createProject(const std::filesystem::path& projectDir, const Project& project);
    bool buildProject(const std::filesystem::path& projectDir,
                      const BuildOptions& options = {});

    // Compile a single .frc file directly to an executable in outDir.
    // If outDir is empty, the executable is placed next to the source.
    bool buildSingleFile(const std::filesystem::path& sourceFile,
                         const std::filesystem::path& outDir,
                         const BuildOptions& options = {});
}