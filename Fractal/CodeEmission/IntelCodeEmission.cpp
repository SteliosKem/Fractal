// IntelCodeEmission.cpp
// Contains the IntelCodeEmmision method implementations
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "IntelCodeEmission.h"

namespace Fractal {
	const std::string& IntelCodeEmission::emit(const InstructionList* instructions) {
		m_instructions = instructions;

		writeLine("section .text");

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
		case InstructionType::Negate: emitNegation(instruction); return;
		case InstructionType::BitwiseNot: emitBitwiseNot(instruction); return;
		//case InstructionType::Return: emitReturn(); return;
		default: return;
		}
	}

	void IntelCodeEmission::emitFunctionDefinition(InstructionPtr instruction) {
		std::shared_ptr<FunctionDefInstruction> functionDefinition = static_pointer_cast<FunctionDefInstruction>(instruction);
		
		writeLine("global " + functionDefinition->name);
		label(functionDefinition->name);

		emitFunctionPrologue(functionDefinition->stackAlloc);

		for (auto instruction : functionDefinition->instructions)
			emitInstruction(instruction);
		emitFunctionEpilogue();
		emitReturn();
	}

	void IntelCodeEmission::emitFunctionPrologue(uint64_t stackAlloc) {
		writeILine("push rbp");
		writeILine("mov rbp, rsp");
		writeILine("sub rsp, " + std::to_string(stackAlloc));
	}

	void IntelCodeEmission::emitFunctionEpilogue() {
		writeILine("mov rsp, rbp");
		writeILine("pop rbp");
	}

	void IntelCodeEmission::emitMove(InstructionPtr instruction) {
		std::shared_ptr<MoveInstruction> moveInstruction = static_pointer_cast<MoveInstruction>(instruction);

		writeILine("mov " + getOperandStr(moveInstruction->destination) + ", " + getOperandStr(moveInstruction->source));
	}

	void IntelCodeEmission::emitNegation(InstructionPtr instruction) {
		std::shared_ptr<NegateInstruction> negInstruction = static_pointer_cast<NegateInstruction>(instruction);

		writeILine("neg " + getOperandStr(negInstruction->source));
	}

	void IntelCodeEmission::emitBitwiseNot(InstructionPtr instruction) {
		std::shared_ptr<NegateInstruction> negInstruction = static_pointer_cast<NegateInstruction>(instruction);

		writeILine("not " + getOperandStr(negInstruction->source));
	}

	void IntelCodeEmission::emitReturn() {
		writeILine("ret");
	}

	std::string IntelCodeEmission::getRegister(OperandPtr operand) {
		std::shared_ptr<RegisterOperand> reg = static_pointer_cast<RegisterOperand>(operand);
		switch (reg->reg) {
		case Register::AX: return "eax";
		case Register::R10: return "r10d";
		default: return "";
		}
	}

	std::string IntelCodeEmission::getOperandStr(OperandPtr operand) {
		switch (operand->getType())
		{
		case OperandType::IntegerConstant: return std::to_string(static_pointer_cast<IntegerConstant>(operand)->integer);
		case OperandType::Register: return getRegister(static_pointer_cast<RegisterOperand>(operand));
		case OperandType::Temp: return getTemp(operand);
		default:
			return "";
		}
	}

	std::string IntelCodeEmission::getTemp(OperandPtr operand) {
		std::shared_ptr<TempOperand> temp = static_pointer_cast<TempOperand>(operand);
		return "DWORD [rbp - " + std::to_string(temp->stackOffest) + "]";
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