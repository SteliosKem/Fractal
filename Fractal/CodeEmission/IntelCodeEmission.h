// IntelCodeEmission.h
// Contains the IntelCodeEmmision class definition, with which intel assembly is generated from an InstructionList
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once

#include "Utilities.h"
#include "CodeGeneration/CodeGenerator.h"

namespace Fractal {
	class IntelCodeEmission {
	public:
		IntelCodeEmission() = default;

		const std::string& emit(const InstructionList* instructions);
		const std::string& output() const;
	private:
		// -- FROM INSTRUCTION LIST --
		void emitInstruction(InstructionPtr instruction);
		void emitFunctionDefinition(InstructionPtr instruction);
		void emitMove(InstructionPtr instruction);
		void emitReturn();

		std::string getOperandStr(OperandPtr operand);

		// -- UTILITY --

		void write(const std::string& text);
		void writeLine(const std::string& line);

		// Write Indented Line
		void writeILine(const std::string& line);
		void label(const std::string& name);
		std::string getSize(Size size);
		std::string getRegister(OperandPtr operand);
	private:
		const InstructionList* m_instructions;

		std::string m_output;
	};
}