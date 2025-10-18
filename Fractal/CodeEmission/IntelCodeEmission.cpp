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
		case InstructionType::Add: emitAdd(instruction); return;
		case InstructionType::Subtract: emitSub(instruction); return;
		case InstructionType::Multiply: emitMul(instruction); return;
		case InstructionType::Cdq: emitCdq(); return;
		case InstructionType::Divide: emitIdiv(instruction); return;
		case InstructionType::Compare: emitCmp(instruction); return;
		case InstructionType::Set: emitSet(instruction); return;
		//case InstructionType::Return: emitReturn(); return;
		default: return;
		}
	}

	void IntelCodeEmission::emitFunctionDefinition(InstructionPtr instruction) {
		std::shared_ptr<FunctionDefInstruction> functionDefinition = static_pointer_cast<FunctionDefInstruction>(instruction);
		
		writeLine("global _" + functionDefinition->name);
		label("_" + functionDefinition->name);

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

		writeILine((moveInstruction->signExtend ? "movsx " : "mov ")
				+ getOperandStr(moveInstruction->destination) + ", " + getOperandStr(moveInstruction->source));
	}

	void IntelCodeEmission::emitNegation(InstructionPtr instruction) {
		std::shared_ptr<NegateInstruction> negInstruction = static_pointer_cast<NegateInstruction>(instruction);

		writeILine("neg " + getOperandStr(negInstruction->source));
	}

	void IntelCodeEmission::emitBitwiseNot(InstructionPtr instruction) {
		std::shared_ptr<NegateInstruction> negInstruction = static_pointer_cast<NegateInstruction>(instruction);

		writeILine("not " + getOperandStr(negInstruction->source));
	}

	void IntelCodeEmission::emitAdd(InstructionPtr instruction) {
		std::shared_ptr<AddInstruction> addInstruction = static_pointer_cast<AddInstruction>(instruction);

		writeILine("add " + getOperandStr(addInstruction->destination) + ", " + getOperandStr(addInstruction->other));
	}

	void IntelCodeEmission::emitSub(InstructionPtr instruction) {
		std::shared_ptr<SubtractInstruction> subInstruction = static_pointer_cast<SubtractInstruction>(instruction);

		writeILine("sub " + getOperandStr(subInstruction->destination) + ", " + getOperandStr(subInstruction->other));
	}

	void IntelCodeEmission::emitMul(InstructionPtr instruction) {
		std::shared_ptr<SubtractInstruction> subInstruction = static_pointer_cast<SubtractInstruction>(instruction);

		writeILine("imul " + getOperandStr(subInstruction->destination) + ", " + getOperandStr(subInstruction->other));
	}

	void IntelCodeEmission::emitCdq() {
		writeILine("cdq");
	}

	void IntelCodeEmission::emitIdiv(InstructionPtr instruction) {
		std::shared_ptr<DivInstruction> divInstruction = static_pointer_cast<DivInstruction>(instruction);

		writeILine("idiv " + getOperandStr(divInstruction->destination));
	}

	void IntelCodeEmission::emitCmp(InstructionPtr instruction) {
		std::shared_ptr<CompareInstruction> cmpInstruction = static_pointer_cast<CompareInstruction>(instruction);

		writeILine("cmp " + getOperandStr(cmpInstruction->left) + ", " + getOperandStr(cmpInstruction->right));
	}

	void IntelCodeEmission::emitSet(InstructionPtr instruction) {
		std::shared_ptr<SetInstruction> setInstruction = static_pointer_cast<SetInstruction>(instruction);

		writeILine("set" + getComparisonType(setInstruction->type) + " " + getOperandStr(setInstruction->destination));
	}

	void IntelCodeEmission::emitReturn() {
		writeILine("ret");
	}

	std::string IntelCodeEmission::getRegister(OperandPtr operand) {
		std::shared_ptr<RegisterOperand> reg = static_pointer_cast<RegisterOperand>(operand);
		switch (reg->reg) {
		case Register::AX: return "eax";
		case Register::BX: return "ebx";
		case Register::DX: return "edx";
		case Register::R10: return "r10d";
		case Register::R11: return "r11d";
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

	std::string getSizeMemory(Size size) {
		switch(size) {
			case Size::Byte: return "BYTE";
			case Size::Word: return "WORD";
			case Size::DWord: return "DWORD";
			case Size::QWord: return "QWORD";
		}
	}

	std::string IntelCodeEmission::getTemp(OperandPtr operand) {
		std::shared_ptr<TempOperand> temp = static_pointer_cast<TempOperand>(operand);
		return getSizeMemory(temp->size) + " [rbp - " + std::to_string(temp->stackOffest) + "]";
	}

	std::string IntelCodeEmission::getComparisonType(ComparisonType type) {
		switch(type) {
			case ComparisonType::Equal: return "e";
			case ComparisonType::NotEqual: return "ne";
			case ComparisonType::Greater: return "g";
			case ComparisonType::GreaterEqual: return "ge";
			case ComparisonType::Less: return "l";
			case ComparisonType::LessEqual: return "le";
		}
		return "";
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