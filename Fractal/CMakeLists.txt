# Compiler core, split into a static library so unit tests can link against it
# without rebuilding every translation unit. The Fractal executable is then a
# thin wrapper that links the core and provides only the CLI entry point.

set(FRACTAL_CORE_SOURCES
    Common.h
    Utilities.h
    Utilities.cpp
    json.hpp

    Types/Type.h

    Error/Position.h
    Error/Error.h
    Error/Error.cpp

    Lexer/Token.h
    Lexer/Lexer.h
    Lexer/Lexer.cpp

    Parser/Nodes.h
    Parser/Parser.h
    Parser/Parser.cpp

    Analysis/SemanticAnalyzer.h
    Analysis/SemanticAnalyzer.cpp

    CodeGeneration/Instructions.h
    CodeGeneration/CodeGenerator.h
    CodeGeneration/CodeGenerator.cpp

    CodeEmission/CodeEmitter.h
    CodeEmission/IntelCodeEmission.h
    CodeEmission/IntelCodeEmission.cpp

    Sequence/Sequence.h
    Sequence/Sequence.cpp
)

add_library(FractalCore STATIC ${FRACTAL_CORE_SOURCES})
target_include_directories(FractalCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET FractalCore PROPERTY CXX_STANDARD 20)
set_property(TARGET FractalCore PROPERTY CXX_STANDARD_REQUIRED ON)

if(MSVC)
    target_compile_options(FractalCore PRIVATE /W4)
else()
    target_compile_options(FractalCore PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-switch)
endif()

add_executable(Fractal Fractal.cpp Fractal.h)
target_link_libraries(Fractal PRIVATE FractalCore)
set_property(TARGET Fractal PROPERTY CXX_STANDARD 20)

install(TARGETS Fractal DESTINATION bin)
