# CMakeList.txt : CMake project for Fractal, include source and define
# project specific logic here.

add_executable(Fractal
    Fractal.cpp
    Fractal.h
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

target_include_directories(Fractal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET Fractal PROPERTY CXX_STANDARD 20)
set_property(TARGET Fractal PROPERTY CXX_STANDARD_REQUIRED ON)

if(MSVC)
    target_compile_options(Fractal PRIVATE /W4)
else()
    target_compile_options(Fractal PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-switch)
endif()

install(TARGETS Fractal DESTINATION bin)
