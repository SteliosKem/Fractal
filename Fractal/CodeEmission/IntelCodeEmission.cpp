// IntelCodeEmission.cpp
// Contains the IntelCodeEmmision method implementations
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "IntelCodeEmission.h"

namespace Fractal {
	const std::string& IntelCodeEmission::emit(const InstructionList* instructions, const std::vector<std::string>* externals, Platform platform) {
		m_platform = platform;
		m_instructions = instructions;
		m_externals = externals;

		std::string writeExt = "extern ";
		for (auto& str : *externals)
			writeExt += str + ", ";

		writeLine(writeExt);

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
		case InstructionType::Jump: emitJmp(instruction); return;
		case InstructionType::Label: emitLabel(instruction); return;
		case InstructionType::Call: emitCall(instruction); return;
		case InstructionType::Return: emitFunctionEpilogue(); emitReturn(); return;
		case InstructionType::Push: emitPush(instruction); return;
		default: return;
		}
	}

	void IntelCodeEmission::emitFunctionDefinition(InstructionPtr instruction) {
		std::shared_ptr<FunctionDefInstruction> functionDefinition = static_pointer_cast<FunctionDefInstruction>(instruction);
		
		if (m_platform == Platform::Mac) {
			writeLine("global _" + functionDefinition->name);
			label("_" + functionDefinition->name);
		}
		else if (m_platform == Platform::Win) {
			writeLine("global " + functionDefinition->name);
			label(functionDefinition->name);
		}

		emitFunctionPrologue(functionDefinition->stackAlloc);

		for (auto instruction : functionDefinition->instructions)
			emitInstruction(instruction);
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

	void IntelCodeEmission::emitJmp(InstructionPtr instruction) {
		std::shared_ptr<JumpInstruction> jmpInstruction = static_pointer_cast<JumpInstruction>(instruction);

		writeILine("j" + getComparisonType(jmpInstruction->type) + " " + jmpInstruction->label);
	}

	void IntelCodeEmission::emitLabel(InstructionPtr instruction) {
		std::shared_ptr<Label> label = static_pointer_cast<Label>(instruction);

		writeLine(label->name + ":");
	}

	void IntelCodeEmission::emitCall(InstructionPtr instruction) {
		std::shared_ptr<CallInstruction> call = static_pointer_cast<CallInstruction>(instruction);

		writeILine("call " + call->func);
	}

	void IntelCodeEmission::emitPush(InstructionPtr instruction) {
		std::shared_ptr<PushInstruction> push = static_pointer_cast<PushInstruction>(instruction);

		writeILine("push " + getOperandStr(push->src));
	}

	void IntelCodeEmission::emitReturn() {
		writeILine("ret");
	}

	std::string IntelCodeEmission::getRegister(OperandPtr operand) {
		std::shared_ptr<RegisterOperand> reg = static_pointer_cast<RegisterOperand>(operand);
		std::string toRet = "";
		if ((uint8_t)reg->reg < 8) {
			switch (reg->size) {
			case Size::QWord: toRet = "r"; break;
			case Size::DWord: toRet = "e"; break;
			}
		}
		switch (reg->reg) {
			case Register::AX: toRet += "ax"; break;
			case Register::BX: toRet += "bx"; break;
			case Register::CX: toRet += "cx"; break;
			case Register::DX: toRet += "dx"; break;
			case Register::DI: toRet += "di"; break;
			case Register::SI: toRet += "si"; break;
			case Register::BP: toRet += "bp"; break;
			case Register::SP: toRet += "sp"; break;
			case Register::R8: toRet += "r8"; break;
			case Register::R9: toRet += "r9"; break;
			case Register::R10: toRet += "r10"; break;
			case Register::R11: toRet += "r11"; break;
			default: return "";
		}
		if ((uint8_t)reg->reg >= 8) {
			switch (reg->size) {
			case Size::QWord: break;
			case Size::DWord: toRet += "d"; break;
			}
		}
		return toRet;
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
		std::string sign = temp->stackOffest < 0 ? "+" : "-";
		return getSizeMemory(temp->size) + " [rbp " + sign + " " + std::to_string(abs(temp->stackOffest)) + "]";
	}

	std::string IntelCodeEmission::getComparisonType(ComparisonType type) {
		switch(type) {
			case ComparisonType::Equal: return "e";
			case ComparisonType::NotEqual: return "ne";
			case ComparisonType::Greater: return "g";
			case ComparisonType::GreaterEqual: return "ge";
			case ComparisonType::Less: return "l";
			case ComparisonType::LessEqual: return "le";
			case ComparisonType::None: return "mp";
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