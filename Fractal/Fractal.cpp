#include "Fractal.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

class InputParser {
public:
    InputParser(int argc, char **argv) {
        for (int i = 1; i < argc; ++i)
            m_tokens.emplace_back(argv[i]);
    }

    // Returns the value following `option` in argv, or empty string.
    std::string getCmdOption(const std::string &option) const {
        auto it = std::find(m_tokens.begin(), m_tokens.end(), option);
        if (it != m_tokens.end() && ++it != m_tokens.end())
            return *it;
        return {};
    }

    bool cmdOptionExists(const std::string &option) const {
        return std::find(m_tokens.begin(), m_tokens.end(), option) != m_tokens.end();
    }

    // Returns the first positional argument (one not consumed as a flag value
    // and not starting with '-'). Used to pick out subcommands like `create`
    // or `build`.
    std::string positional(size_t index = 0) const {
        size_t seen = 0;
        for (const auto &t : m_tokens) {
            if (!t.empty() && t[0] == '-') continue;
            if (seen == index) return t;
            ++seen;
        }
        return {};
    }

private:
    std::vector<std::string> m_tokens;
};

void printUsage() {
    std::cout
        << "Fractal Command Usage:\n"
        << "  -h, --help                        Show this help\n"
        << "  -v, --verbose                     Dump tokens, AST, IR, and asm\n"
        << "  create <project_name>             Create a project in the cwd\n"
        << "  build                             Build the project in the cwd\n"
        << "  -f <file>                         Compile a single .frc file\n"
        << "  -f <file> -o <out_dir>            Compile to a custom output dir\n";
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        printUsage();
        return EXIT_SUCCESS;
    }

    InputParser input(argc, argv);

    if (input.cmdOptionExists("-h") || input.cmdOptionExists("--help")) {
        printUsage();
        return EXIT_SUCCESS;
    }

    Fractal::BuildOptions options;
    options.verbose = input.cmdOptionExists("-v") || input.cmdOptionExists("--verbose");

    const std::string projectName = input.getCmdOption("create");
    if (!projectName.empty()) {
        Fractal::createProject(
            std::filesystem::current_path(),
            Fractal::Project{projectName, "src", "build", Fractal::defaultArchitecture()});
        return EXIT_SUCCESS;
    }

    const std::string sourceFile = input.getCmdOption("-f");
    if (!sourceFile.empty()) {
        const std::string outDir = input.getCmdOption("-o");
        return Fractal::buildSingleFile(sourceFile, outDir, options)
            ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (input.cmdOptionExists("build")) {
        return Fractal::buildProject(std::filesystem::current_path(), options)
            ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    std::cout << "Unknown command. Run 'Fractal --help' for usage.\n";
    return EXIT_FAILURE;
}
