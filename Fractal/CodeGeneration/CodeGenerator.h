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
		void generateVariableDefinition(StatementPtr definition, InstructionList* instructions);

		// -- STATEMENTS --
		void generateStatement(StatementPtr statement, InstructionList* instructions);
		void generateReturnStatement(StatementPtr statement, InstructionList* instructions);
		void generateCompoundStatement(StatementPtr statement, InstructionList* instructions);
		void generateExpressionStatement(StatementPtr statement, InstructionList* instructions);
		void generateIfStatement(StatementPtr statement, InstructionList* instructions);

		// -- EXPRESSIONS --
		OperandPtr generateExpression(ExpressionPtr expression, InstructionList* instructions);
		OperandPtr generateUnaryOperation(ExpressionPtr expression, InstructionList* instructions);
		OperandPtr generateIntConstant(ExpressionPtr expression, InstructionList* instructions);
		OperandPtr generateBinaryOperation(ExpressionPtr expression, InstructionList* instructions);
		OperandPtr generateArithmeticOperation(std::shared_ptr<BinaryOperation> expression, InstructionList* instructions);
		OperandPtr generateRelational(std::shared_ptr<BinaryOperation> expression, InstructionList* instructions);
		OperandPtr generateLogical(std::shared_ptr<BinaryOperation> expression, InstructionList* instructions);
		OperandPtr generateAssignment(ExpressionPtr expression, InstructionList* instructions);
		OperandPtr getIdentifier(ExpressionPtr expression);
		OperandPtr idiv(std::shared_ptr<BinaryOperation> division, InstructionList* instructions);

		// -- INSTRUCTIONS --
		InstructionPtr move(OperandPtr source, OperandPtr destination);
		InstructionPtr negate(OperandPtr source);
		InstructionPtr bitwiseNot(OperandPtr source);
		InstructionPtr add(OperandPtr destination, OperandPtr other);
		InstructionPtr sub(OperandPtr destination, OperandPtr other);
		InstructionPtr mul(OperandPtr destination, OperandPtr other);
		InstructionPtr cmp(OperandPtr left, OperandPtr right);
		InstructionPtr set(OperandPtr operand, ComparisonType type);
		InstructionPtr jmp(const std::string& label, ComparisonType type);
		OperandPtr reg(Register register_, Size size = Size::DWord);
		OperandPtr intConst(int64_t integer);
		InstructionPtr label(const std::string& name);

		// -- STACK --
		int64_t allocateStack(Size size);
		void popStack(uint64_t ammount);

		// -- VALIDATE INSTRUCTIONS --
		void validateInstructions(InstructionList* instructions);
		void validateFunction(InstructionPtr instruction);
		void validateMove(InstructionList* instructions, size_t i);
		void validateAdd(InstructionList* instructions, size_t i);
		void validateSub(InstructionList* instructions, size_t i);
		void validateMul(InstructionList* instructions, size_t i);
		void validateDiv(InstructionList* instructions, size_t i);
		void validateCmp(InstructionList* instructions, size_t i);

		void validateMoveOperands(InstructionList* instructions, size_t i, OperandPtr source, OperandPtr* destination);
		void validateBinOperands(InstructionList* instructions, size_t i, OperandPtr source, OperandPtr* other);

		// -- LABELS --
		uint64_t generateComparisonIndex();
		uint64_t generateIfIndex();
		uint64_t generateLoopIndex();
	private:
		InstructionList m_instructions{};
		ProgramFile m_program{};
		int64_t m_currentStackIndex{};
		std::unordered_map<std::string, OperandPtr> m_localVarMap{};
		uint64_t m_currentComparisonIndex{};
		uint64_t m_currentIfIndex{};
		uint64_t m_currentLoopIndex{};

		ErrorHandler* m_errorHandler{ nullptr };
	};
}