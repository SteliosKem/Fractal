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

		validateInstructions(&m_instructions);

		return m_instructions;
	}

	void CodeGenerator::generateDefinition(DefinitionPtr definition, InstructionList* instructions) {
		switch (definition->getType()) {
			case NodeType::FunctionDefinition: generateFunctionDefinition(definition, instructions);
			default: return;
		}
	}

	void CodeGenerator::generateFunctionDefinition(DefinitionPtr definition, InstructionList* instructions) {
		m_currentStackIndex = 0;
		std::shared_ptr<FunctionDefinition> func = static_pointer_cast<FunctionDefinition>(definition);
		std::shared_ptr<FunctionDefInstruction> mainFunc = std::make_shared<FunctionDefInstruction>("main", InstructionList{});

		mainFunc->name = func->nameToken.value;
		generateStatement(func->functionBody, &mainFunc->instructions);

		instructions->push_back(mainFunc);
		mainFunc->stackAlloc = m_currentStackIndex;
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

		instructions->push_back(move(generateExpression(returnStatement->expression, instructions), reg(Register::AX)));
		instructions->push_back(std::make_shared<ReturnInstruction>());
	}

	OperandPtr CodeGenerator::generateExpression(ExpressionPtr expression, InstructionList* instructions) {
		switch (expression->getType()) {
			case NodeType::IntegerLiteral: return generateIntConstant(expression, instructions);
			case NodeType::UnaryOperation: return generateUnaryOperation(expression, instructions);
			default: return nullptr;
		}
	}

	OperandPtr CodeGenerator::generateUnaryOperation(ExpressionPtr expression, InstructionList* instructions) {
		std::shared_ptr<UnaryOperation> unaryOp = static_pointer_cast<UnaryOperation>(expression);
		std::shared_ptr<TempOperand> destination = std::make_shared<TempOperand>(allocateStack(Size::DWord));

		instructions->push_back(move(generateExpression(unaryOp->expression, instructions), destination));

		switch (unaryOp->operatorToken.type) {
		case MINUS:
			instructions->push_back(negate(destination));
			break;
		case TILDE:
			instructions->push_back(bitwiseNot(destination));
			break;
		}

		return destination;
	}

	OperandPtr CodeGenerator::generateIntConstant(ExpressionPtr expression, InstructionList* instructions) {
		std::shared_ptr<IntegerLiteral> intLiteral = static_pointer_cast<IntegerLiteral>(expression);

		return intConst(intLiteral->value);
	}


	InstructionPtr CodeGenerator::move(OperandPtr source, OperandPtr destination) {
		return std::make_shared<MoveInstruction>(source, destination);
	}

	InstructionPtr CodeGenerator::negate(OperandPtr source) {
		return std::make_shared<NegateInstruction>(source);
	}

	InstructionPtr CodeGenerator::bitwiseNot(OperandPtr source) {
		return std::make_shared<BitwiseNotInstruction>(source);
	}

	OperandPtr CodeGenerator::reg(Register register_) {
		return std::make_shared<RegisterOperand>(register_);
	}

	OperandPtr CodeGenerator::intConst(int64_t integer) {
		return std::make_shared<IntegerConstant>(integer);
	}

	int64_t CodeGenerator::allocateStack(Size size) {
		m_currentStackIndex += (int)size;
		return m_currentStackIndex;
	}

	void popStack(uint64_t ammount);

	void CodeGenerator::validateInstructions(InstructionList* instructions) {
		for(size_t i = 0; i < instructions->size(); i++) {
			switch ((*instructions)[i]->getType()) {
			case InstructionType::FunctionDefinition: validateFunction((*instructions)[i]); break;
			case InstructionType::Move: validateMove(instructions, i); break;
			}
		}
	}

	void CodeGenerator::validateFunction(InstructionPtr instruction) {
		std::shared_ptr<FunctionDefInstruction> func = static_pointer_cast<FunctionDefInstruction>(instruction);
		validateInstructions(&func->instructions);
	}

	void CodeGenerator::validateMove(InstructionList* instructions, size_t i) {
		std::shared_ptr<MoveInstruction> moveInstruction = static_pointer_cast<MoveInstruction>((*instructions)[i]);
		if (moveInstruction->source->getType() == OperandType::Temp && moveInstruction->destination->getType() == OperandType::Temp) {
			OperandPtr scratchReg = reg(Register::R10);
			OperandPtr destination = moveInstruction->destination;
			moveInstruction->destination = scratchReg;
			instructions->emplace(instructions->begin() + i + 1, move(scratchReg, destination));
		}
	}
}