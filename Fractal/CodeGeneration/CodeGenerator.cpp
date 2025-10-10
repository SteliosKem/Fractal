// CodeGenerator.h
// Contains the CodeGenerator method implementations
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "CodeGenerator.h"

namespace Fractal {
	const InstructionList& CodeGenerator::generate(const ProgramFile& program) {
		m_program = program;
		m_instructions = {};
		for (auto definition : program.definitions)
			generateDefinition(definition, &m_instructions);

		std::shared_ptr<FunctionDefInstruction> mainFunc = std::make_shared<FunctionDefInstruction>("main", InstructionList{});
		for (auto statement : program.statements)
			generateStatement(statement, &mainFunc->instructions);

		return m_instructions;
	}

	void CodeGenerator::generateDefinition(DefinitionPtr definition, InstructionList* instructions) {
		switch (definition->getType()) {
			case NodeType::FunctionDefinition: generateFunctionDefinition(definition, instructions);
			default: return;
		}
	}

	void CodeGenerator::generateFunctionDefinition(DefinitionPtr definition, InstructionList* instructions) {
		std::shared_ptr<FunctionDefinition> func = static_pointer_cast<FunctionDefinition>(definition);
		std::shared_ptr<FunctionDefInstruction> mainFunc = std::make_shared<FunctionDefInstruction>("main", InstructionList{});

		mainFunc->name = func->nameToken.value;
		generateStatement(func->functionBody, &mainFunc->instructions);

		instructions->push_back(mainFunc);
	}

	void CodeGenerator::generateStatement(StatementPtr statement, InstructionList* instructions) {
		switch (statement->getType()) {
			case NodeType::ReturnStatement: generateReturnStatement(statement, instructions); return;
			case NodeType::CompoundStatement: generateCompoundStatement(statement, instructions); return;
			default: return;
		}
	}

	void CodeGenerator::generateCompoundStatement(StatementPtr statement, InstructionList* instructions) {
		std::shared_ptr<CompoundStatement> compoundStatement = static_pointer_cast<CompoundStatement>(statement);

		for (auto statement_ : compoundStatement->statements)
			generateStatement(statement_, instructions);
	}

	void CodeGenerator::generateReturnStatement(StatementPtr statement, InstructionList* instructions) {
		std::shared_ptr<ReturnStatement> returnStatement = static_pointer_cast<ReturnStatement>(statement);

		generateExpression(returnStatement->expression, instructions);
		instructions->push_back(std::make_shared<ReturnInstruction>());
	}

	void CodeGenerator::generateExpression(ExpressionPtr expression, InstructionList* instructions) {
		switch (expression->getType()) {
			case NodeType::IntegerLiteral: generateIntConstant(expression, instructions);
			default: return;
		}
	}

	void CodeGenerator::generateIntConstant(ExpressionPtr expression, InstructionList* instructions) {
		std::shared_ptr<IntegerLiteral> intLiteral = static_pointer_cast<IntegerLiteral>(expression);

		instructions->push_back(move(intConst(intLiteral->value), reg(Register::AX)));
	}


	InstructionPtr CodeGenerator::move(OperandPtr source, OperandPtr destination) {
		return std::make_shared<MoveInstruction>(source, destination);
	}

	OperandPtr CodeGenerator::reg(Register register_) {
		return std::make_shared<RegisterOperand>(register_);
	}

	OperandPtr CodeGenerator::intConst(int64_t integer) {
		return std::make_shared<IntegerConstant>(integer);
	}
}