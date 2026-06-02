# Tests/CMakeLists.txt
# Builds the fractal_tests executable, which links against FractalCore.

add_executable(fractal_tests
    test_framework.h
    test_main.cpp
    test_lexer.cpp
    test_parser.cpp
    test_analyzer.cpp
    test_e2e.cpp
)

target_link_libraries(fractal_tests PRIVATE FractalCore)
target_include_directories(fractal_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Inject the absolute path to the .frc programs so e2e tests can locate them
# regardless of where CTest is invoked from.
target_compile_definitions(fractal_tests PRIVATE
    FRACTAL_TEST_PROGRAMS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/programs"
)

set_property(TARGET fractal_tests PROPERTY CXX_STANDARD 20)
set_property(TARGET fractal_tests PROPERTY CXX_STANDARD_REQUIRED ON)

add_test(NAME fractal_tests COMMAND fractal_tests)
