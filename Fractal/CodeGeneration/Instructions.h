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
		Negate,
		Not
	};

	enum class OperandType {
		Operand,
		IntegerConstant,
		Register,
		Temp
	};

	enum class Register {
		AX,
		R10
	};

	enum class Size : uint8_t {
		Byte = 1,
		Word = 2,
		DWord = 4,
		QWord = 8
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
		virtual OperandType getType() const override { return OperandType::IntegerConstant; }
		void print() const override { std::cout << integer; }
	public:
		int64_t integer;
	};

	class RegisterOperand : public Operand {
	public:
		RegisterOperand(Register reg) : reg{ reg } {}
		virtual OperandType getType() const override { return OperandType::Register; }
		void print() const override { std::cout << '%' << (int)reg; }
	public:
		Register reg;
	};

	class TempOperand : public Operand {
	public:
		TempOperand(int64_t stackOffest) : stackOffest{ stackOffest } {}
		virtual OperandType getType() const override { return OperandType::Temp; }
		void print() const override { std::cout << "Stack access " << stackOffest; }
	public:
		int64_t stackOffest;
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
			std::cout << "Function " << name << " stack alloc " << stackAlloc << ":\n";
			for (auto instruction : instructions) {
				std::cout << "    ";
				instruction->print();
			}
			std::cout << '\n';
		}
	public:
		std::string name;
		InstructionList instructions;
		uint64_t stackAlloc;
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

	class NegateInstruction : public Instruction {
	public:
		NegateInstruction(OperandPtr source) : source{ source } {}
		INSTR_TYPE(Negate)
			virtual void print() const override {
			std::cout << "Negate ";
			source->print();
		}
	public:
		OperandPtr source;
	};

	class NotInstruction : public Instruction {
	public:
		NotInstruction(OperandPtr source) : source{ source } {}
		INSTR_TYPE(Not)
			virtual void print() const override {
			std::cout << "Not ";
			source->print();
		}
	public:
		OperandPtr source;
	};

	class ReturnInstruction : public Instruction {
	public:
		ReturnInstruction() = default;
		INSTR_TYPE(Return)
		virtual void print() const override {
			std::cout << "Ret\n";
		}
	};
}