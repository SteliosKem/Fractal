// test_framework.h
// Minimal hand-rolled test framework. Single header, no external deps.
//
//   TEST_CASE("name")        — registers a test; body follows in braces.
//   CHECK(expr)              — non-fatal assertion. Test continues on failure.
//   CHECK_EQ(a, b)           — non-fatal equality, prints both sides on failure.
//   REQUIRE(expr)            — fatal assertion. Returns from the test on failure.
//   REQUIRE_EQ(a, b)         — fatal equality.
//
// All failures are counted and printed with file:line. main() returns the
// number of failing tests (0 on full success), so CTest sees a clean pass/fail.

#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace tf {

struct TestCase {
    std::string name;
    std::string file;
    int line;
    std::function<void()> fn;
};

inline std::vector<TestCase> &registry() {
    static std::vector<TestCase> r;
    return r;
}

// Per-test failure counter. Reset between tests by the runner.
inline int &currentFailures() {
    static int f = 0;
    return f;
}

struct Registrar {
    Registrar(std::string name, std::string file, int line, std::function<void()> fn) {
        registry().push_back({std::move(name), std::move(file), line, std::move(fn)});
    }
};

} // namespace tf

#define TF_CONCAT2(a, b) a##b
#define TF_CONCAT(a, b) TF_CONCAT2(a, b)

#define TEST_CASE(name)                                                                       \
    static void TF_CONCAT(tf_test_, __LINE__)();                                              \
    static ::tf::Registrar TF_CONCAT(tf_reg_, __LINE__){                                      \
        (name), __FILE__, __LINE__, TF_CONCAT(tf_test_, __LINE__)};                           \
    static void TF_CONCAT(tf_test_, __LINE__)()

#define CHECK(expr)                                                                           \
    do {                                                                                      \
        if (!(expr)) {                                                                        \
            std::cerr << "  CHECK failed: " << #expr << "\n    at " << __FILE__ << ":"        \
                      << __LINE__ << "\n";                                                    \
            ++::tf::currentFailures();                                                        \
        }                                                                                     \
    } while (0)

#define CHECK_EQ(a, b)                                                                        \
    do {                                                                                      \
        auto _a = (a);                                                                        \
        auto _b = (b);                                                                        \
        if (!(_a == _b)) {                                                                    \
            std::ostringstream _o;                                                            \
            _o << "  CHECK_EQ failed: " << #a << " == " << #b << "\n    " << #a << " = "     \
               << _a << "\n    " << #b << " = " << _b << "\n    at " << __FILE__ << ":"      \
               << __LINE__ << "\n";                                                           \
            std::cerr << _o.str();                                                            \
            ++::tf::currentFailures();                                                        \
        }                                                                                     \
    } while (0)

#define REQUIRE(expr)                                                                         \
    do {                                                                                      \
        if (!(expr)) {                                                                        \
            std::cerr << "  REQUIRE failed: " << #expr << "\n    at " << __FILE__ << ":"      \
                      << __LINE__ << "\n";                                                    \
            ++::tf::currentFailures();                                                        \
            return;                                                                           \
        }                                                                                     \
    } while (0)

#define REQUIRE_EQ(a, b)                                                                      \
    do {                                                                                      \
        auto _a = (a);                                                                        \
        auto _b = (b);                                                                        \
        if (!(_a == _b)) {                                                                    \
            std::ostringstream _o;                                                            \
            _o << "  REQUIRE_EQ failed: " << #a << " == " << #b << "\n    " << #a << " = "   \
               << _a << "\n    " << #b << " = " << _b << "\n    at " << __FILE__ << ":"      \
               << __LINE__ << "\n";                                                           \
            std::cerr << _o.str();                                                            \
            ++::tf::currentFailures();                                                        \
            return;                                                                           \
        }                                                                                     \
    } while (0)
