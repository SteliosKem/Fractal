// Instructions.h
// Contains Instruction related definitions
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include "Common.h"

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

#define TYPE(x) InstructionType getType() const override { return InstructionType::x; }

	class Operand {
	public:
		virtual ~Operand() = default;
		virtual OperandType getType() const { return OperandType::Operand; }
	};

	using OperandPtr = std::shared_ptr<Operand>;

	class IntegerConstant : public Operand {
	public:
		IntegerConstant(int64_t integer) : integer{ integer } {}
		virtual OperandType getType() const { return OperandType::IntegerConstant; }
	public:
		int64_t integer;
	};

	class RegisterOperand : public Operand {
	public:
		RegisterOperand(Register reg) : reg{ reg } {}
		virtual OperandType getType() const { return OperandType::Register; }
	public:
		Register reg;
	};

	class Instruction {
	public:
		virtual ~Instruction() = default;
		virtual InstructionType getType() const { return InstructionType::Instruction; }
	};

	using InstructionPtr = std::shared_ptr<Instruction>;
	using InstructionList = std::vector<InstructionPtr>;

	class FunctionDefinition : public Instruction {
	public:
		FunctionDefinition(const std::string& name, const InstructionList& instructions)
			: name{ name }, instructions{ instructions } {}
		TYPE(FunctionDefinition)
	public:
		std::string name;
		InstructionList instructions;
	};

	class MoveInstruction : public Instruction {
	public:
		MoveInstruction(OperandPtr source, OperandPtr destination)
			: source{ source }, destination{ destination } {}
		TYPE(Move)
	public:
		OperandPtr source;
		OperandPtr destination;
	};

	class Return : public Instruction {
	public:
		Return() = default;
		TYPE(Return)
	};

}