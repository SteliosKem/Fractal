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

		// -- EXPRESSIONS --
		OperandPtr generateExpression(ExpressionPtr expression, InstructionList* instructions);
		OperandPtr generateUnaryOperation(ExpressionPtr expression, InstructionList* instructions);
		OperandPtr generateIntConstant(ExpressionPtr expression, InstructionList* instructions);
		OperandPtr generateBinaryOperation(ExpressionPtr expression, InstructionList* instructions);

		// -- INSTRUCTIONS --
		InstructionPtr move(OperandPtr source, OperandPtr destination);
		InstructionPtr negate(OperandPtr source);
		InstructionPtr bitwiseNot(OperandPtr source);
		InstructionPtr add(OperandPtr destination, OperandPtr other);
		OperandPtr reg(Register register_);
		OperandPtr intConst(int64_t integer);

		// -- STACK --
		int64_t allocateStack(Size size);
		void popStack(uint64_t ammount);

		// -- VALIDATE INSTRUCTIONS --
		void validateInstructions(InstructionList* instructions);
		void validateFunction(InstructionPtr instruction);
		void validateMove(InstructionList* instructions, size_t i);
		void validateAdd(InstructionList* instructions, size_t i);
	private:
		InstructionList m_instructions{};
		ProgramFile m_program{};
		int64_t m_currentStackIndex{};
		ErrorHandler* m_errorHandler{ nullptr };
	};
}