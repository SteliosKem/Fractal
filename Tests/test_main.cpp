// test_main.cpp
// Test runner. Executes every registered TEST_CASE, prints a per-test result,
// and exits with the number of failing tests (0 == all green).

#include "test_framework.h"

#include <chrono>
#include <iomanip>
#include <iostream>

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    auto &tests = tf::registry();
    int passed = 0;
    int failed = 0;

    std::cout << "Running " << tests.size() << " test(s)\n";

    auto runStart = std::chrono::steady_clock::now();

    for (const auto &tc : tests) {
        const int before = tf::currentFailures();
        std::cout << "[ RUN  ] " << tc.name << "\n";

        try {
            tc.fn();
        } catch (const std::exception &e) {
            std::cerr << "  threw std::exception: " << e.what() << "\n";
            ++tf::currentFailures();
        } catch (...) {
            std::cerr << "  threw unknown exception\n";
            ++tf::currentFailures();
        }

        if (tf::currentFailures() > before) {
            std::cout << "[ FAIL ] " << tc.name << "\n";
            ++failed;
        } else {
            std::cout << "[  OK  ] " << tc.name << "\n";
            ++passed;
        }
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - runStart)
                       .count();

    std::cout << "\n" << passed << " passed, " << failed << " failed, " << elapsed << " ms\n";
    return failed;
}
