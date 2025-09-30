# CMakeList.txt : CMake project for Fractal, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Fractal "Fractal.cpp" "Fractal.h" "Utilities.h" "Common.h" "Utilities.cpp" "Lexer/Lexer.h" "Lexer/Token.h" "Lexer/Type.h" "Error/Position.h" "Lexer/Lexer.cpp" "Error/Error.h" "Error/Error.cpp" "Parser/Nodes.h" "Parser/Parser.h" "Parser/Parser.cpp" "Analysis/SemanticAnalyzer.h" "Analysis/SemanticAnalyzer.cpp")

target_include_directories(Fractal PRIVATE "/")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Fractal PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
