// CodeGenerator.h
// Contains the CodeGenerator Class definition
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include "Error/Error.h"
#include "Instructions.h"

namespace Fractal {
	class CodeGenerator {
	public:
		CodeGenerator(ErrorHandler* errorHandler) : m_errorHandler{ errorHandler } {}


	private:
		ErrorHandler* m_errorHandler{ nullptr };
	};
}