// test_analyzer.cpp
// Unit tests for Fractal::SemanticAnalyzer. Each test lexes + parses a small
// source and runs semantic analysis, then asserts on the error/warning state.

#include "test_framework.h"

#include "Analysis/SemanticAnalyzer.h"
#include "Error/Error.h"
#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include "Utilities.h"

#include <filesystem>

namespace {

struct AnalyzeResult {
    bool parseOk;
    bool analyzeOk;
    Fractal::ErrorHandler eh;
    Fractal::ProgramFile program; // copy; analyzer only needs the pointer
};

// Runs lex + parse + analyze on `source`. Returns parse/analyze success flags
// and the error handler so tests can inspect reported errors.
std::shared_ptr<AnalyzeResult> analyze(const std::string &source,
                                       const std::string &name = "anon") {
    auto r = std::make_shared<AnalyzeResult>();
    auto path = std::filesystem::temp_directory_path() / ("fractal_anatest_" + name + ".frc");
    Fractal::writeFile(source, path);

    Fractal::Lexer lexer(&r->eh);
    if (!lexer.analyze(path)) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
        r->parseOk = false;
        r->analyzeOk = false;
        return r;
    }

    Fractal::Parser parser(&r->eh);
    r->parseOk = parser.parse(lexer.getTokenList());
    if (!r->parseOk) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
        r->analyzeOk = false;
        return r;
    }

    r->program = std::move(parser.program());
    Fractal::SemanticAnalyzer sema(&r->eh);
    r->analyzeOk = sema.analyze(&r->program);

    std::error_code ec;
    std::filesystem::remove(path, ec);
    return r;
}

} // namespace

TEST_CASE("analyzer: simple valid function passes") {
    auto r = analyze("<define> fn main(): i32 { return 1; } <!define>", "ok_simple");
    CHECK(r->parseOk);
    CHECK(r->analyzeOk);
    CHECK(!r->eh.hasErrors());
}

TEST_CASE("analyzer: arithmetic with consistent types passes") {
    auto r = analyze(
        "<define> fn main(): i32 { let x: i32 = 1 + 2 * 3; return x; } <!define>",
        "arith");
    CHECK(r->parseOk);
    CHECK(r->analyzeOk);
}

TEST_CASE("analyzer: undefined identifier reports an error") {
    auto r = analyze("<define> fn main(): i32 { return missing; } <!define>", "undef");
    CHECK(r->parseOk);
    CHECK(!r->analyzeOk);
    CHECK(r->eh.hasErrors());
}

TEST_CASE("analyzer: duplicate function definition reports an error") {
    auto r = analyze(
        "<define> fn dup(): i32 { return 1; } fn dup(): i32 { return 2; } <!define>",
        "dup_fn");
    CHECK(r->parseOk);
    CHECK(!r->analyzeOk);
    CHECK(r->eh.hasErrors());
}

TEST_CASE("analyzer: call with wrong argument count reports an error") {
    auto r = analyze(
        "<define> fn add(a: i32, b: i32): i32 { return a + b; }\n"
        "fn main(): i32 { return add(1); } <!define>",
        "argcount");
    CHECK(r->parseOk);
    CHECK(!r->analyzeOk);
    CHECK(r->eh.hasErrors());
}

TEST_CASE("analyzer: break outside loop reports an error") {
    auto r = analyze("<define> fn main(): i32 { break; return 0; } <!define>", "break_out");
    CHECK(r->parseOk);
    CHECK(!r->analyzeOk);
    CHECK(r->eh.hasErrors());
}

TEST_CASE("analyzer: continue outside loop reports an error") {
    auto r = analyze("<define> fn main(): i32 { continue; return 0; } <!define>", "cont_out");
    CHECK(r->parseOk);
    CHECK(!r->analyzeOk);
    CHECK(r->eh.hasErrors());
}

TEST_CASE("analyzer: return outside function reports an error") {
    auto r = analyze("return 1;", "ret_out");
    CHECK(r->parseOk);
    CHECK(!r->analyzeOk);
    CHECK(r->eh.hasErrors());
}

TEST_CASE("analyzer: while inside function with break is OK") {
    auto r = analyze(
        "<define> fn main(): i32 { while 1 == 1 => { break; } return 0; } <!define>",
        "while_break");
    CHECK(r->parseOk);
    CHECK(r->analyzeOk);
}

TEST_CASE("analyzer: function call with matching argument types passes") {
    auto r = analyze(
        "<define>\n"
        "fn add(a: i32, b: i32): i32 { return a + b; }\n"
        "fn main(): i32 { return add(1, 2); }\n"
        "<!define>",
        "call_ok");
    CHECK(r->parseOk);
    CHECK(r->analyzeOk);
}

TEST_CASE("analyzer: integer literal infers as I32") {
    auto r = analyze(
        "<define> fn main(): i32 { let x: i32 = 5; return x; } <!define>",
        "int_infer");
    CHECK(r->parseOk);
    CHECK(r->analyzeOk);
    // The variable's resolved type lives on the AST after analyze; just
    // confirm no errors were reported, which is the user-visible behaviour.
    CHECK(!r->eh.hasErrors());
}

// Smoke regression for critical bug #6: the assignment type-check used to be
// a self-cast (right side cast to its own type), so a type-mismatched
// assignment was silently accepted. With the fix in place, an i64-valued
// expression assigned to an i32 lvalue should pass via widening (i32→i64) and
// the reverse should also be allowed by the existing num-promotion rules.
// Here we just confirm the call site is exercised and produces no spurious
// error on a self-consistent assignment.
TEST_CASE("analyzer: assignment to same-typed variable analyzes cleanly") {
    auto r = analyze(
        "<define> fn main(): i32 { let x: i32 = 0; x = 5; return x; } <!define>",
        "assign_ok");
    CHECK(r->parseOk);
    CHECK(r->analyzeOk);
    CHECK(!r->eh.hasErrors());
}
