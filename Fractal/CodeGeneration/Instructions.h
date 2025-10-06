// Instructions.h
// Contains Instruction related definitions
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include "Common.h"
#include <iostream>

namespace Fractal {
	enum class InstructionType {
		// Definitions
		FunctionDefinition,

		// Instructions
		Instruction,
		Move,
		Return,
	};

	enum class OperandType {
		Operand,
		IntegerConstant,
		Register
	};

	enum class Register {
		AX
	};

#define INSTR_TYPE(x) InstructionType getType() const override { return InstructionType::x; }

	class Operand {
	public:
		virtual ~Operand() = default;
		virtual OperandType getType() const { return OperandType::Operand; }
		virtual void print() const {}
	};

	using OperandPtr = std::shared_ptr<Operand>;

	class IntegerConstant : public Operand {
	public:
		IntegerConstant(int64_t integer) : integer{ integer } {}
		virtual OperandType getType() const { return OperandType::IntegerConstant; }
		void print() const override { std::cout << integer; }
	public:
		int64_t integer;
	};

	class RegisterOperand : public Operand {
	public:
		RegisterOperand(Register reg) : reg{ reg } {}
		virtual OperandType getType() const { return OperandType::Register; }
		void print() const override { std::cout << '%' << (int)reg; }
	public:
		Register reg;
	};

	class Instruction {
	public:
		virtual ~Instruction() = default;
		virtual InstructionType getType() const { return InstructionType::Instruction; }
		virtual void print() const {}
	};

	using InstructionPtr = std::shared_ptr<Instruction>;
	using InstructionList = std::vector<InstructionPtr>;

	class FunctionDefInstruction : public Instruction {
	public:
		FunctionDefInstruction(const std::string& name, const InstructionList& instructions)
			: name{ name }, instructions{ instructions } {}
		INSTR_TYPE(FunctionDefinition)
		virtual void print() const override {
			std::cout << "Function " + name + ":\n";
			for (auto instruction : instructions) {
				std::cout << "    ";
				instruction->print();
			}
			std::cout << '\n';
		}
	public:
		std::string name;
		InstructionList instructions;
	};

	class MoveInstruction : public Instruction {
	public:
		MoveInstruction(OperandPtr source, OperandPtr destination)
			: source{ source }, destination{ destination } {}
		INSTR_TYPE(Move)
		virtual void print() const override {
			std::cout << "Move ";
			source->print();
			std::cout << ", ";
			destination->print();
			std::cout << '\n';
		}
	public:
		OperandPtr source;
		OperandPtr destination;
	};

	class Return : public Instruction {
	public:
		Return() = default;
		INSTR_TYPE(Return)
		virtual void print() const override {
			std::cout << "Ret\n";
		}
	};
}