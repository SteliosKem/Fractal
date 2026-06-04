// test_e2e.cpp
// End-to-end compiler tests. Each test invokes Sequence::buildSingleFile to
// compile a .frc program, runs the resulting executable, and asserts on the
// exit code.
//
// These tests shell out to nasm and gcc (via Sequence). If either binary is
// not on PATH, the tests skip with a clear message rather than fail — this
// keeps the suite usable on machines without a full assembler/linker stack.
//
// The path to Tests/programs/ is injected at build time via the
// FRACTAL_TEST_PROGRAMS_DIR macro (see Tests/CMakeLists.txt).

#include "test_framework.h"

#include "Sequence/Sequence.h"

#include <cstdlib>
#include <filesystem>
#include <string>

#ifndef FRACTAL_TEST_PROGRAMS_DIR
#define FRACTAL_TEST_PROGRAMS_DIR "."
#endif

namespace {

// True if `which <tool>` succeeds — i.e. the tool is on PATH.
bool toolAvailable(const std::string &tool) {
    return std::system(("which " + tool + " > /dev/null 2>&1").c_str()) == 0;
}

bool toolchainAvailable() {
    static bool checked = false;
    static bool available = false;
    if (!checked) {
        available = toolAvailable("nasm") && toolAvailable("gcc");
        checked = true;
        if (!available) {
            std::cout << "  [skip reason] nasm and/or gcc not on PATH; "
                         "e2e tests will be skipped.\n";
        }
    }
    return available;
}

// POSIX exit codes from std::system are in the high byte. Extract them so
// tests compare against the raw integer the .frc program returned.
int extractExitCode(int rawStatus) {
#if defined(_WIN32) || defined(_WIN64)
    return rawStatus;
#else
    // WEXITSTATUS shifts the high byte down. If the child died by signal we
    // return a sentinel that won't match any normal exit code.
    if ((rawStatus & 0x7f) != 0) return -1; // killed by signal
    return (rawStatus >> 8) & 0xff;
#endif
}

// Compiles `progName.frc` from the programs directory and runs the resulting
// binary. Returns the exit code, or -2 if compilation itself failed.
int compileAndRun(const std::string &progName) {
    auto programDir = std::filesystem::path(FRACTAL_TEST_PROGRAMS_DIR);
    auto sourceFile = programDir / (progName + ".frc");

    auto outDir = std::filesystem::temp_directory_path() / ("fractal_e2e_" + progName);
    std::error_code ec;
    std::filesystem::remove_all(outDir, ec);

    Fractal::BuildOptions opts;
    opts.verbose = false;
    opts.keepAsm = false;

    if (!Fractal::buildSingleFile(sourceFile, outDir, opts)) {
        return -2;
    }

    auto binary = outDir / progName;
    int raw = std::system(("\"" + binary.string() + "\"").c_str());
    return extractExitCode(raw);
}

} // namespace

TEST_CASE("e2e: return 42") {
    if (!toolchainAvailable()) return;
    CHECK_EQ(compileAndRun("return_42"), 42);
}

TEST_CASE("e2e: arithmetic — (2+3)*4-1 == 19") {
    if (!toolchainAvailable()) return;
    CHECK_EQ(compileAndRun("arithmetic"), 19);
}

TEST_CASE("e2e: if true branch returns 7") {
    if (!toolchainAvailable()) return;
    CHECK_EQ(compileAndRun("if_true"), 7);
}

TEST_CASE("e2e: if/else taking the false branch returns 11") {
    if (!toolchainAvailable()) return;
    CHECK_EQ(compileAndRun("if_else_false"), 11);
}

TEST_CASE("e2e: function call add(20, 5) returns 25") {
    if (!toolchainAvailable()) return;
    CHECK_EQ(compileAndRun("function_call"), 25);
}

TEST_CASE("e2e: while loop summing 1..10 returns 55") {
    if (!toolchainAvailable()) return;
    CHECK_EQ(compileAndRun("while_count"), 55);
}

// Regression for the AddressOf / Dereference codegen pair: `&a` produces
// a pointer through `lea`, `@p` loads through it via an IndirectOperand.
// Validates that the full pipeline (parse → analyze → codegen → emit →
// nasm → gcc → run) handles pointer round-trips end-to-end.
TEST_CASE("e2e: address-of + dereference round-trip returns 13") {
    if (!toolchainAvailable()) return;
    CHECK_EQ(compileAndRun("address_deref"), 13);
}

// Regression for the `@p = rhs` store path. Verifies that the store goes
// through the pointer to the pointed-to memory (mutating `a`), not into a
// local copy. If the codegen accidentally treated `@p` as an rvalue location
// the test would return 5 instead of 42.
TEST_CASE("e2e: store through pointer updates original variable") {
    if (!toolchainAvailable()) return;
    CHECK_EQ(compileAndRun("assign_through_pointer"), 42);
}
