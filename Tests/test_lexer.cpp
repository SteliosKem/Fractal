// test_lexer.cpp
// Unit tests for Fractal::Lexer. Sources are written to a temp file so the
// lexer's filesystem-based analyze() path is exercised end-to-end.

#include "test_framework.h"

#include "Error/Error.h"
#include "Lexer/Lexer.h"
#include "Utilities.h"

#include <cstdio>
#include <filesystem>

namespace {

// Writes `source` to a unique temporary .frc file and returns the path. The
// caller is responsible for removing it (or letting the OS reap /tmp).
std::filesystem::path writeSource(const std::string &source, const std::string &name) {
    auto path = std::filesystem::temp_directory_path() / ("fractal_test_" + name + ".frc");
    Fractal::writeFile(source, path);
    return path;
}

// Runs the lexer on the given source and returns the resulting token list. If
// the lexer reports an error, `hadError` is set to true.
std::vector<Fractal::Token> lex(const std::string &source, bool &hadError,
                                const std::string &name = "anon") {
    Fractal::ErrorHandler eh;
    Fractal::Lexer lexer(&eh);
    auto path = writeSource(source, name);
    bool ok = lexer.analyze(path);
    hadError = !ok;
    auto tokens = lexer.getTokenList();
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return tokens;
}

} // namespace

TEST_CASE("lexer: empty source emits only EOF") {
    bool err;
    auto tokens = lex("", err, "empty");
    CHECK(!err);
    REQUIRE_EQ(tokens.size(), size_t{1});
    CHECK_EQ((int)tokens[0].type, (int)Fractal::SPECIAL_EOF);
}

TEST_CASE("lexer: single-character tokens") {
    bool err;
    auto tokens = lex("(){}[]+-*/;,.:", err, "singles");
    CHECK(!err);
    // Last token must be EOF (regression for the missing-EOF crash, bug #4).
    REQUIRE(!tokens.empty());
    CHECK_EQ((int)tokens.back().type, (int)Fractal::SPECIAL_EOF);
    CHECK_EQ((int)tokens[0].type, (int)Fractal::LEFT_PARENTHESIS);
    CHECK_EQ((int)tokens[1].type, (int)Fractal::RIGHT_PARENTHESIS);
    CHECK_EQ((int)tokens[2].type, (int)Fractal::LEFT_BRACE);
    CHECK_EQ((int)tokens[3].type, (int)Fractal::RIGHT_BRACE);
    CHECK_EQ((int)tokens[4].type, (int)Fractal::LEFT_BRACKET);
    CHECK_EQ((int)tokens[5].type, (int)Fractal::RIGHT_BRACKET);
    CHECK_EQ((int)tokens[6].type, (int)Fractal::PLUS);
    CHECK_EQ((int)tokens[7].type, (int)Fractal::MINUS);
    CHECK_EQ((int)tokens[8].type, (int)Fractal::STAR);
    CHECK_EQ((int)tokens[9].type, (int)Fractal::SLASH);
}

TEST_CASE("lexer: double-character tokens") {
    bool err;
    auto tokens = lex("== != <= >= += -= *= /= -> =>", err, "doubles");
    CHECK(!err);
    REQUIRE(tokens.size() >= 10);
    CHECK_EQ((int)tokens[0].type, (int)Fractal::EQUAL_EQUAL);
    CHECK_EQ((int)tokens[1].type, (int)Fractal::BANG_EQUAL);
    CHECK_EQ((int)tokens[2].type, (int)Fractal::LESS_EQUAL);
    CHECK_EQ((int)tokens[3].type, (int)Fractal::GREATER_EQUAL);
    CHECK_EQ((int)tokens[4].type, (int)Fractal::PLUS_EQUAL);
    CHECK_EQ((int)tokens[5].type, (int)Fractal::MINUS_EQUAL);
    CHECK_EQ((int)tokens[6].type, (int)Fractal::STAR_EQUAL);
    CHECK_EQ((int)tokens[7].type, (int)Fractal::SLASH_EQUAL);
    CHECK_EQ((int)tokens[8].type, (int)Fractal::ARROW);
    CHECK_EQ((int)tokens[9].type, (int)Fractal::DOUBLE_ARROW);
}

TEST_CASE("lexer: keywords map to their TokenType") {
    bool err;
    auto tokens = lex("let const fn if else while loop return break continue", err, "kw");
    CHECK(!err);
    REQUIRE(tokens.size() >= 10);
    CHECK_EQ((int)tokens[0].type, (int)Fractal::LET);
    CHECK_EQ((int)tokens[1].type, (int)Fractal::CONST);
    CHECK_EQ((int)tokens[2].type, (int)Fractal::FUNCTION);
    CHECK_EQ((int)tokens[3].type, (int)Fractal::IF);
    CHECK_EQ((int)tokens[4].type, (int)Fractal::ELSE);
    CHECK_EQ((int)tokens[5].type, (int)Fractal::WHILE);
    CHECK_EQ((int)tokens[6].type, (int)Fractal::LOOP);
    CHECK_EQ((int)tokens[7].type, (int)Fractal::RETURN);
    CHECK_EQ((int)tokens[8].type, (int)Fractal::BREAK);
    CHECK_EQ((int)tokens[9].type, (int)Fractal::CONTINUE);
}

TEST_CASE("lexer: identifiers are not keywords") {
    bool err;
    auto tokens = lex("myVar _other a123 letter", err, "ident");
    CHECK(!err);
    REQUIRE(tokens.size() >= 4);
    for (size_t i = 0; i < 4; ++i)
        CHECK_EQ((int)tokens[i].type, (int)Fractal::IDENTIFIER);
    CHECK_EQ(tokens[0].value, std::string("myVar"));
    CHECK_EQ(tokens[1].value, std::string("_other"));
    CHECK_EQ(tokens[2].value, std::string("a123"));
    // "letter" starts with "let" — confirms we lex maximal-munch, not prefix-match.
    CHECK_EQ(tokens[3].value, std::string("letter"));
    CHECK_EQ((int)tokens[3].type, (int)Fractal::IDENTIFIER);
}

TEST_CASE("lexer: integer and float literals are distinguished") {
    bool err;
    auto tokens = lex("42 3.14 0 100", err, "nums");
    CHECK(!err);
    REQUIRE(tokens.size() >= 4);
    CHECK_EQ((int)tokens[0].type, (int)Fractal::TYPE_INTEGER);
    CHECK_EQ(tokens[0].value, std::string("42"));
    CHECK_EQ((int)tokens[1].type, (int)Fractal::TYPE_FLOAT);
    CHECK_EQ(tokens[1].value, std::string("3.14"));
    CHECK_EQ((int)tokens[2].type, (int)Fractal::TYPE_INTEGER);
    CHECK_EQ((int)tokens[3].type, (int)Fractal::TYPE_INTEGER);
}

TEST_CASE("lexer: string and character literals") {
    bool err;
    auto tokens = lex("\"hello\" 'a'", err, "strs");
    CHECK(!err);
    REQUIRE(tokens.size() >= 2);
    CHECK_EQ((int)tokens[0].type, (int)Fractal::STRING_LITERAL);
    CHECK_EQ(tokens[0].value, std::string("hello"));
    CHECK_EQ((int)tokens[1].type, (int)Fractal::CHARACTER_LITERAL);
    CHECK_EQ(tokens[1].value, std::string("a"));
}

TEST_CASE("lexer: single-line comments are skipped") {
    bool err;
    auto tokens = lex("// just a comment\n42", err, "linecmt");
    CHECK(!err);
    REQUIRE(tokens.size() >= 1);
    CHECK_EQ((int)tokens[0].type, (int)Fractal::TYPE_INTEGER);
    CHECK_EQ(tokens[0].value, std::string("42"));
}

TEST_CASE("lexer: multi-line comments are skipped") {
    bool err;
    auto tokens = lex("/* ignore\nthis whole\nblock */ 99", err, "blockcmt");
    CHECK(!err);
    REQUIRE(tokens.size() >= 1);
    CHECK_EQ((int)tokens[0].type, (int)Fractal::TYPE_INTEGER);
    CHECK_EQ(tokens[0].value, std::string("99"));
}

// Regression test for critical bug #3 — a line comment with no trailing
// newline before EOF used to spin forever (`|| c == '\0'` instead of `&&`).
TEST_CASE("lexer: line comment terminated by EOF does not hang") {
    bool err;
    auto tokens = lex("// comment with no newline", err, "linecmt_eof");
    CHECK(!err);
    // Should produce only the EOF token.
    REQUIRE_EQ(tokens.size(), size_t{1});
    CHECK_EQ((int)tokens[0].type, (int)Fractal::SPECIAL_EOF);
}

// Regression test for critical bug #4 — analyze() used to never push EOF
// when the source ended cleanly, so the parser would read past the end.
TEST_CASE("lexer: EOF token is always the last token") {
    bool err;
    auto tokens = lex("let x = 5", err, "eof_last");
    CHECK(!err);
    REQUIRE(!tokens.empty());
    CHECK_EQ((int)tokens.back().type, (int)Fractal::SPECIAL_EOF);
}

TEST_CASE("lexer: line numbers advance on newlines") {
    bool err;
    auto tokens = lex("a\nb\nc", err, "lines");
    CHECK(!err);
    REQUIRE(tokens.size() >= 3);
    CHECK_EQ((int)tokens[0].position.line, 1);
    CHECK_EQ((int)tokens[1].position.line, 2);
    CHECK_EQ((int)tokens[2].position.line, 3);
}

TEST_CASE("lexer: unknown character reports an error") {
    bool err;
    auto tokens = lex("?", err, "unknown");
    CHECK(err);
    (void)tokens;
}

TEST_CASE("lexer: unterminated string reports an error") {
    bool err;
    auto tokens = lex("\"never closed", err, "unterm");
    CHECK(err);
    (void)tokens;
}

TEST_CASE("lexer: typed keywords (i32, f64, bool) become KEY_* tokens") {
    bool err;
    auto tokens = lex("i32 i64 u32 f64 bool null", err, "typekw");
    CHECK(!err);
    REQUIRE(tokens.size() >= 6);
    CHECK_EQ((int)tokens[0].type, (int)Fractal::KEY_I32);
    CHECK_EQ((int)tokens[1].type, (int)Fractal::KEY_I64);
    CHECK_EQ((int)tokens[2].type, (int)Fractal::KEY_U32);
    CHECK_EQ((int)tokens[3].type, (int)Fractal::KEY_F64);
    CHECK_EQ((int)tokens[4].type, (int)Fractal::KEY_BOOL);
    CHECK_EQ((int)tokens[5].type, (int)Fractal::KEY_NULL);
}
