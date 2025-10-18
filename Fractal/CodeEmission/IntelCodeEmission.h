// IntelCodeEmission.h
// Contains the IntelCodeEmmision class definition, with which intel assembly is generated from an InstructionList
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once

#include "Utilities.h"
#include "CodeGeneration/CodeGenerator.h"

namespace Fractal {
	enum class Platform {
		Win,
		Mac
	};

	class IntelCodeEmission {
	public:
		IntelCodeEmission() = default;

		const std::string& emit(const InstructionList* instructions, Platform platform);
		const std::string& output() const;
	private:
		// -- FROM INSTRUCTION LIST --
		void emitInstruction(InstructionPtr instruction);
		void emitFunctionDefinition(InstructionPtr instruction);
		void emitFunctionPrologue(uint64_t stackAlloc);
		void emitFunctionEpilogue();
		void emitMove(InstructionPtr instruction);
		void emitNegation(InstructionPtr instruction);
		void emitBitwiseNot(InstructionPtr instruction);
		void emitAdd(InstructionPtr instruction);
		void emitSub(InstructionPtr instruction);
		void emitMul(InstructionPtr instruction);
		void emitCdq();
		void emitCmp(InstructionPtr instruction);
		void emitSet(InstructionPtr instruction);
		void emitIdiv(InstructionPtr instruction);
		void emitReturn();

		std::string getOperandStr(OperandPtr operand);
		std::string getTemp(OperandPtr operand);

		// -- UTILITY --

		void write(const std::string& text);
		void writeLine(const std::string& line);
		std::string getComparisonType(ComparisonType type);

		// Write Indented Line
		void writeILine(const std::string& line);
		void label(const std::string& name);
		std::string getSize(Size size);
		std::string getRegister(OperandPtr operand);
	private:
		const InstructionList* m_instructions;
		Platform m_platform;
		std::string m_output;
	};
}