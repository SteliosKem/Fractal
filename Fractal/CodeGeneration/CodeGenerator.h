// CodeGenerator.h
// Contains the CodeGenerator Class definition
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include "Error/Error.h"
#include "Instructions.h"
#include "Parser/Nodes.h"

namespace Fractal {
	class CodeGenerator {
	public:
		CodeGenerator(ErrorHandler* errorHandler) : m_errorHandler{ errorHandler } {}

		const InstructionList& generate(const ProgramFile& program);
		const InstructionList& instructions() const { return m_instructions; }
	private:
		// -- DEFINITIONS --
		void generateDefinition(DefinitionPtr definition, InstructionList* instructions);
		void generateFunctionDefinition(DefinitionPtr definition, InstructionList* instructions);

		// -- STATEMENTS --
		void generateStatement(StatementPtr statement, InstructionList* instructions);
		void generateReturnStatement(StatementPtr statement, InstructionList* instructions);
		void generateCompoundStatement(StatementPtr statement, InstructionList* instructions);
		OperandPtr generateConstant();

		// -- EXPRESSIONS --
		void generateExpression(ExpressionPtr expression, InstructionList* instructions);
		void generateIntConstant(ExpressionPtr expression, InstructionList* instructions);

		// -- INSTRUCTIONS --
		InstructionPtr move(OperandPtr source, OperandPtr destination);
		OperandPtr reg(Register register_);
		OperandPtr intConst(int64_t integer);
	private:
		InstructionList m_instructions{};
		ProgramFile m_program{};
		ErrorHandler* m_errorHandler{ nullptr };
	};
}