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

    bool createProject(const std::filesystem::path& projectDir, const Project& project);
    bool buildProject(const std::filesystem::path& projectDir);
}