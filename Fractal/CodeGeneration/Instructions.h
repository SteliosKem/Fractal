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
		BitwiseNot,
		Add,
		Subtract,
		Multiply,
		Divide,
		Remainder,
		Cdq,
		Compare,
		Set,
		Jump,
		Label
	};

	enum class OperandType {
		Operand,
		IntegerConstant,
		Register,
		Temp
	};

	enum class Register {
		AX,
		BX,
		DX,
		R10,
		R11,
		R9
	};

	enum class Size : uint8_t {
		Byte = 1,
		Word = 2,
		DWord = 4,
		QWord = 8
	};

	enum class ComparisonType {
		Equal,
		NotEqual,
		Greater,
		GreaterEqual,
		Less,
		LessEqual,
		None
	};

#define INSTR_TYPE(x) InstructionType getType() const override { return InstructionType::x; }

	class Operand {
	public:
		virtual ~Operand() = default;
		virtual OperandType getType() const { return OperandType::Operand; }
		virtual Size getSize() const { return Size::DWord; }
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
		RegisterOperand(Register reg, Size size = Size::DWord) : reg{ reg }, size{ size } {}
		virtual OperandType getType() const override { return OperandType::Register; }
		void print() const override { std::cout << '%' << (int)reg; }
		virtual Size getSize() const override { return size; }
	public:
		Register reg;
		Size size;
	};

	class TempOperand : public Operand {
	public:
		TempOperand(int64_t stackOffest, Size size) : stackOffest{ stackOffest }, size{ size } {}
		virtual OperandType getType() const override { return OperandType::Temp; }
		void print() const override { std::cout << "Stack access " << stackOffest; }
		virtual Size getSize() const override { return size; }
	public:
		int64_t stackOffest;
		Size size;
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
		uint64_t stackAlloc{ 0 };
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
		bool signExtend{ false };
	};

	class Label : public Instruction {
	public:
		Label(const std::string& name) : name{ name } {}
		INSTR_TYPE(Label)
			virtual void print() const override {
			std::cout << "label " << name << ":\n";
		}
	public:
		std::string name;
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

	class BitwiseNotInstruction : public Instruction {
	public:
		BitwiseNotInstruction(OperandPtr source) : source{ source } {}
		INSTR_TYPE(BitwiseNot)
			virtual void print() const override {
			std::cout << "BW-Not ";
			source->print();
		}
	public:
		OperandPtr source;
	};

	class AddInstruction : public Instruction {
	public:
		AddInstruction(OperandPtr destination, OperandPtr other) : destination{ destination }, other{ other } {}
		INSTR_TYPE(Add)
			virtual void print() const override {
			std::cout << "add ";
			other->print();
			std::cout << " to ";
			destination->print();
		}
	public:
		OperandPtr destination;
		OperandPtr other;
	};

	class SubtractInstruction : public Instruction {
	public:
		SubtractInstruction(OperandPtr destination, OperandPtr other) : destination{ destination }, other{ other } {}
		INSTR_TYPE(Subtract)
			virtual void print() const override {
			std::cout << "sub ";
			other->print();
			std::cout << " from ";
			destination->print();
		}
	public:
		OperandPtr destination;
		OperandPtr other;
	};

	class MultiplyInstruction : public Instruction {
	public:
		MultiplyInstruction(OperandPtr destination, OperandPtr other) : destination{ destination }, other{ other } {}
		INSTR_TYPE(Multiply)
			virtual void print() const override {
			std::cout << "mul ";
			other->print();
			std::cout << " with ";
			destination->print();
		}
	public:
		OperandPtr destination;
		OperandPtr other;
	};

	class DivInstruction : public Instruction {
	public:
		DivInstruction(OperandPtr destination) : destination{ destination } {}
		INSTR_TYPE(Divide)
			virtual void print() const override {
			std::cout << "idiv ";
			destination->print();
		}
	public:
		OperandPtr destination;
	};

	class CompareInstruction : public Instruction {
	public:
		CompareInstruction(OperandPtr left, OperandPtr right) : left{ left }, right{ right } {}
		INSTR_TYPE(Compare)
			virtual void print() const override {
			std::cout << "cmp ";
			left->print();
			std::cout << " with ";
			right->print();
		}
	public:
		OperandPtr left;
		OperandPtr right;
	};

	class SetInstruction : public Instruction {
	public:
		SetInstruction(OperandPtr destination, ComparisonType type) : destination{ destination }, type{ type } {}
		INSTR_TYPE(Set)
			virtual void print() const override {
			std::cout << "set ";
			destination->print();
		}
	public:
		OperandPtr destination;
		ComparisonType type;
	};

	class JumpInstruction : public Instruction {
	public:
		JumpInstruction(const std::string& label, ComparisonType type) : label{ label }, type{ type } {}
		INSTR_TYPE(Jump)
			virtual void print() const override {
			std::cout << "jump " << label;
		}
	public:
		std::string label;
		ComparisonType type;
	};

	class ReturnInstruction : public Instruction {
	public:
		ReturnInstruction() = default;
		INSTR_TYPE(Return)
		virtual void print() const override {
			std::cout << "Ret\n";
		}
	};

	class CdqInstruction : public Instruction {
	public:
		CdqInstruction() = default;
		INSTR_TYPE(Cdq)
		virtual void print() const override {
			std::cout << "cdq\n";
		}
	};
}