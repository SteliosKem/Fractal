// test_parser.cpp
// Unit tests for Fractal::Parser. Each test lexes a small source into tokens,
// runs the parser, and inspects the resulting AST shape.

#include "test_framework.h"

#include "Error/Error.h"
#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include "Utilities.h"

#include <filesystem>

namespace {

// Lex `source` into a token list. Tests assume no lex errors; if a parse test
// needs a malformed source, use lexExpectError() instead.
Fractal::TokenList lexSource(const std::string &source, const std::string &name,
                             Fractal::ErrorHandler &eh) {
    auto path = std::filesystem::temp_directory_path() / ("fractal_parsetest_" + name + ".frc");
    Fractal::writeFile(source, path);
    Fractal::Lexer lexer(&eh);
    lexer.analyze(path);
    auto tokens = lexer.getTokenList();
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return tokens;
}

// Convenience: run a full lex+parse and return the populated ProgramFile.
// The ProgramFile owns unique_ptr<AST> trees, so ParseResult must be
// move-only — we never need to copy it in a test.
struct ParseResult {
    bool ok;
    Fractal::ProgramFile program;
};

ParseResult parse(const std::string &source, const std::string &name = "anon") {
    Fractal::ErrorHandler eh;
    auto tokens = lexSource(source, name, eh);
    Fractal::Parser parser(&eh);
    bool ok = parser.parse(tokens);
    return ParseResult{ok, std::move(parser.program())};
}

// Test-only cast helper: with unique_ptr AST nodes we can no longer use
// std::static_pointer_cast, so the equivalent is a raw downcast on .get().
template <class T, class U>
T *as(const std::unique_ptr<U> &p) {
    return static_cast<T *>(p.get());
}

} // namespace

TEST_CASE("parser: empty source produces empty program") {
    auto r = parse("", "empty");
    CHECK(r.ok);
    CHECK_EQ(r.program.statements.size(), size_t{0});
    CHECK_EQ(r.program.definitions.size(), size_t{0});
}

TEST_CASE("parser: function definition inside <define> block") {
    auto r = parse("<define> fn foo(): i32 { return 1; } <!define>", "fn_simple");
    CHECK(r.ok);
    REQUIRE_EQ(r.program.definitions.size(), size_t{1});
    auto &def = r.program.definitions[0];
    CHECK(dynamic_cast<Fractal::FunctionDefinition*>(def.get()) != nullptr);
    auto fn = as<Fractal::FunctionDefinition>(def);
    CHECK_EQ(fn->nameToken.value, std::string("foo"));
    CHECK_EQ(fn->parameterList.size(), size_t{0});
}

TEST_CASE("parser: function with parameters") {
    auto r = parse("<define> fn add(a: i32, b: i32): i32 { return a + b; } <!define>", "fn_params");
    CHECK(r.ok);
    REQUIRE_EQ(r.program.definitions.size(), size_t{1});
    auto fn = as<Fractal::FunctionDefinition>(r.program.definitions[0]);
    REQUIRE_EQ(fn->parameterList.size(), size_t{2});
    CHECK_EQ(fn->parameterList[0]->nameToken.value, std::string("a"));
    CHECK_EQ(fn->parameterList[1]->nameToken.value, std::string("b"));
}

TEST_CASE("parser: top-level statements outside <define> are program statements") {
    auto r = parse("let x: i32 = 5;", "top_let");
    CHECK(r.ok);
    REQUIRE_EQ(r.program.statements.size(), size_t{1});
    CHECK(dynamic_cast<Fractal::VariableDefinition*>(r.program.statements[0].get()) != nullptr);
}

TEST_CASE("parser: const variable") {
    auto r = parse("const PI: i32 = 3;", "const");
    CHECK(r.ok);
    REQUIRE_EQ(r.program.statements.size(), size_t{1});
    auto var = as<Fractal::VariableDefinition>(r.program.statements[0]);
    CHECK(var->isConst);
    CHECK_EQ(var->nameToken.value, std::string("PI"));
}

TEST_CASE("parser: binary precedence — a + b * c groups as a + (b * c)") {
    auto r = parse("let x: i32 = 1 + 2 * 3;", "prec");
    CHECK(r.ok);
    REQUIRE_EQ(r.program.statements.size(), size_t{1});
    auto var = as<Fractal::VariableDefinition>(r.program.statements[0]);
    REQUIRE(var->initializer != nullptr);
    REQUIRE(dynamic_cast<Fractal::BinaryOperation*>(var->initializer.get()) != nullptr);
    auto top = as<Fractal::BinaryOperation>(var->initializer);
    // Top operator is + (lower precedence), right side is the * subtree.
    CHECK_EQ((int)top->operatorToken.type, (int)Fractal::PLUS);
    REQUIRE(dynamic_cast<Fractal::BinaryOperation*>(top->right.get()) != nullptr);
    auto rhs = as<Fractal::BinaryOperation>(top->right);
    CHECK_EQ((int)rhs->operatorToken.type, (int)Fractal::STAR);
}

TEST_CASE("parser: function call expression") {
    auto r = parse("foo(1, 2, 3);", "call");
    CHECK(r.ok);
    REQUIRE_EQ(r.program.statements.size(), size_t{1});
    auto stmt = as<Fractal::ExpressionStatement>(r.program.statements[0]);
    REQUIRE(dynamic_cast<Fractal::Call*>(stmt->expression.get()) != nullptr);
    auto call = as<Fractal::Call>(stmt->expression);
    CHECK_EQ(call->funcToken.value, std::string("foo"));
    CHECK_EQ(call->argumentList.size(), size_t{3});
}

TEST_CASE("parser: if/else statement") {
    auto r = parse("if 1 == 1 => { let x: i32 = 1; } else { let y: i32 = 2; }", "if_else");
    CHECK(r.ok);
    REQUIRE_EQ(r.program.statements.size(), size_t{1});
    REQUIRE(dynamic_cast<Fractal::IfStatement*>(r.program.statements[0].get()) != nullptr);
    auto ifs = as<Fractal::IfStatement>(r.program.statements[0]);
    CHECK(ifs->condition != nullptr);
    CHECK(ifs->ifBody != nullptr);
    CHECK(ifs->elseBody != nullptr);
}

TEST_CASE("parser: while statement") {
    auto r = parse("while 1 < 5 => { break; }", "while");
    CHECK(r.ok);
    REQUIRE_EQ(r.program.statements.size(), size_t{1});
    CHECK(dynamic_cast<Fractal::WhileStatement*>(r.program.statements[0].get()) != nullptr);
}

TEST_CASE("parser: missing semicolon reports an error") {
    auto r = parse("let x: i32 = 1", "missing_semi");
    CHECK(!r.ok);
}

TEST_CASE("parser: <define> block accepts multiple definitions") {
    auto src =
        "<define>\n"
        "fn a(): i32 { return 1; }\n"
        "fn b(): i32 { return 2; }\n"
        "<!define>\n";
    auto r = parse(src, "multi_def");
    CHECK(r.ok);
    CHECK_EQ(r.program.definitions.size(), size_t{2});
}
