// IntelCodeEmission.h
// Contains the IntelCodeEmmision class definition, with which intel assembly is generated from an InstructionList
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once

#include "CodeEmitter.h"
#include "Utilities.h"

namespace Fractal {

	class IntelCodeEmission : public CodeEmitter {
	public:
		IntelCodeEmission() = default;

		const std::string& emit(const InstructionList* instructions, const std::vector<std::string>* externals, Platform platform) override;
		const std::string& output() const override;
	private:
		// -- FROM INSTRUCTION LIST --
		void emitInstruction(const Instruction* instruction);
		void emitFunctionDefinition(const Instruction* instruction);
		void emitFunctionPrologue(uint64_t stackAlloc);
		void emitFunctionEpilogue();
		void emitMove(const Instruction* instruction);
		void emitNegation(const Instruction* instruction);
		void emitBitwiseNot(const Instruction* instruction);
		void emitAdd(const Instruction* instruction);
		void emitSub(const Instruction* instruction);
		void emitMul(const Instruction* instruction);
		void emitCdq();
		void emitCmp(const Instruction* instruction);
		void emitSet(const Instruction* instruction);
		void emitIdiv(const Instruction* instruction);
		void emitJmp(const Instruction* instruction);
		void emitLabel(const Instruction* instruction);
		void emitCall(const Instruction* instruction);
		void emitPush(const Instruction* instruction);
		void emitReturn();

		std::string getOperandStr(OperandPtr operand, Size externalSize = Size::None);
		std::string getTemp(OperandPtr operand, Size externalSize);

		// -- UTILITY --

		// Apply platform-specific symbol mangling. The only public-facing
		// convention difference between PE/ELF and Mach-O is the leading
		// underscore on every C symbol on Mac.
		std::string mangle(const std::string& name) const;

		void write(const std::string& text);
		void writeLine(const std::string& line);
		std::string getComparisonType(ComparisonType type);

		// Write Indented Line
		void writeILine(const std::string& line);
		void label(const std::string& name);
		std::string getSize(Size size);
		std::string getRegister(OperandPtr operand, Size externalSize);
	private:
		const InstructionList* m_instructions;
		Platform m_platform;
		const std::vector<std::string>* m_externals;
		std::string m_output;
	};
}