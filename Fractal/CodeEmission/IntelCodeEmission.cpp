// IntelCodeEmission.cpp
// Contains the IntelCodeEmmision method implementations
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "IntelCodeEmission.h"

namespace Fractal {
	const std::string& IntelCodeEmission::emit(const InstructionList* instructions) {
		m_instructions = instructions;

		for (auto instruction : *instructions)
			emitInstruction(instruction);

		return m_output;
	}

	const std::string& IntelCodeEmission::output() const {
		return m_output;
	}

	std::string IntelCodeEmission::getSize(Size size) {
		switch (size) {
			case Size::Byte: return "BYTE";
			case Size::Word: return "WORD";
			case Size::DWord: return "DWORD";
			case Size::QWord: return "QWORD";
		}
	}

	void IntelCodeEmission::emitInstruction(InstructionPtr instruction) {
		switch (instruction->getType()) {
		case InstructionType::FunctionDefinition: emitFunctionDefinition(instruction); return;
		case InstructionType::Move: emitMove(instruction); return;
		case InstructionType::Return: emitReturn(); return;
		}
	}

	void IntelCodeEmission::emitFunctionDefinition(InstructionPtr instruction) {
		std::shared_ptr<FunctionDefInstruction> functionDefinition = static_pointer_cast<FunctionDefInstruction>(instruction);
		label(functionDefinition->name);

		for (auto instruction : functionDefinition->instructions)
			emitInstruction(instruction);
	}

	void IntelCodeEmission::emitMove(InstructionPtr instruction) {
		std::shared_ptr<MoveInstruction> moveInstruction = static_pointer_cast<MoveInstruction>(instruction);

		writeILine("mov DWORD PTR[eax], " + getOperandStr(moveInstruction->source));
	}

	void IntelCodeEmission::emitReturn() {
		writeILine("ret");
	}

	std::string IntelCodeEmission::getRegister(OperandPtr operand) {
		std::shared_ptr<RegisterOperand> reg = static_pointer_cast<RegisterOperand>(operand);
		switch (reg->reg) {
		case Register::AX: return "eax";
		}
	}

	std::string IntelCodeEmission::getOperandStr(OperandPtr operand) {
		switch (operand->getType())
		{
		case OperandType::IntegerConstant: return std::to_string(static_pointer_cast<IntegerConstant>(operand)->integer);
		case OperandType::Register: return getRegister(static_pointer_cast<RegisterOperand>(operand));
		default:
			break;
		}
	}

	void IntelCodeEmission::write(const std::string& text) {
		m_output += text;
	}

	void IntelCodeEmission::writeLine(const std::string& line) {
		write(line + "\n");
	}

	void IntelCodeEmission::writeILine(const std::string& line) {
		writeLine("    " + line);
	}

	void IntelCodeEmission::label(const std::string& name) {
		writeLine(name + ":");
	}
}