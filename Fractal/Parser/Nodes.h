// Nodes.h
// Contains the definitions of all Abstract Syntax Tree Nodes
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include "Lexer/Token.h"
#include "Common.h"
#include <iostream>

namespace Fractal {
	enum class NodeType {
		Expression,
		IntegerLiteral,
		BinaryOperation,
		UnaryOperation,

		Statement,
		ExpressionStatement,
	};

#define TYPE(x) NodeType getType() const { return x; }

	// Base Classes
	class Expression {
	public:
		virtual ~Expression() = default;
		virtual void print() const {}
		TYPE(NodeType::Expression)
	};

	class Statement {
	public:
		virtual ~Statement() = default;
		virtual void print() const {}
		TYPE(NodeType::Statement)
	};

	using ExpressionPtr = std::unique_ptr<Expression>;
	using StatementPtr = std::unique_ptr<Statement>;

	//
	// Expressions
	//

	class IntegerLiteral : public Expression {
	public:
		IntegerLiteral(int64_t value, const Position& position) : value{ value }, position{ position } {}
		void print() const override { std::cout << value; }
		TYPE(NodeType::IntegerLiteral)
	public:
		int64_t value;
		Position position;
	};

	class UnaryOperation : public Expression {
	public:
		UnaryOperation(const Token& operatorToken, ExpressionPtr expression) : operatorToken{ operatorToken }, expression{ std::move(expression) } {}

		void print() const override {
			std::cout << operatorToken.value << '(';
			expression->print();
			std::cout << ')';
		}

		TYPE(NodeType::UnaryOperation)
	public:
		ExpressionPtr expression;
		Token operatorToken;
	};

	class BinaryOperation : public Expression {
	public:
		BinaryOperation(ExpressionPtr left, const Token& operatorToken, ExpressionPtr right)
			: left{ std::move(left) }, operatorToken{operatorToken}, right{ std::move(right) } {}

		void print() const override {
			std::cout << '(';
			left->print();
			std::cout << ' ' << operatorToken.value << ' ';
			right->print();
			std::cout << ')';
		}

		TYPE(NodeType::BinaryOperation)
	public:
		ExpressionPtr left;
		ExpressionPtr right;
		Token operatorToken;
	};

	//
	// Statements
	//

	class ExpressionStatement : public Statement {
	public:
		ExpressionStatement(ExpressionPtr expression) : expression{ std::move(expression) } {}
		void print() const override {
			std::cout << "->  ";
			expression->print();
			std::cout << '\n';
		}
		TYPE(NodeType::ExpressionStatement)
	public:
		ExpressionPtr expression;
	};

	using StatementList = std::vector<std::unique_ptr<Statement>>;
}