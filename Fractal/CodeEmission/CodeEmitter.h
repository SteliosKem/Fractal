// CodeEmitter.h
// Abstract base class for backend code emitters. Concrete implementations
// (IntelCodeEmission, future ARM emitter, future LLVM-IR emitter, etc.) take
// an InstructionList from the CodeGenerator and produce textual target code.
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once

#include "CodeGeneration/CodeGenerator.h"

namespace Fractal {

class CodeEmitter {
public:
    virtual ~CodeEmitter() = default;

    // Lower the given instruction list to a textual representation for the
    // given platform. The returned string is owned by the emitter and remains
    // valid until the next call to emit().
    virtual const std::string &emit(const InstructionList *instructions,
                                    const std::vector<std::string> *externals,
                                    Platform platform) = 0;

    // The most recently emitted output. Useful when callers need to write it
    // to disk after streaming it elsewhere.
    virtual const std::string &output() const = 0;
};

} // namespace Fractal
