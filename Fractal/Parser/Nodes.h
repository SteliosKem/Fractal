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
		Identifier,
		Call,

		Statement,
		NullStatement,
		CompoundStatement,
		ExpressionStatement,
		ReturnStatement,
		IfStatement,
		LoopStatement,
		WhileStatement,
		BreakStatement,
		ContinueStatement,

		Definition,
		FunctionDefinition,
		VariableDefinition
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
		virtual void print(uint8_t indent = 0) const {}
		TYPE(NodeType::Statement)
	};

	// Derives from Statement in order to be able to create variables in local scope
	class Definition : public Statement {
	public:
		virtual ~Definition() = default;
		virtual void print(uint8_t indent = 0) const override {}
		TYPE(NodeType::Definition)
	};

	using ExpressionPtr = std::unique_ptr<Expression>;
	using StatementPtr = std::unique_ptr<Statement>;
	using DefinitionPtr = std::unique_ptr<Definition>;

	using StatementList = std::vector<std::unique_ptr<Statement>>;
	using DefinitionList = std::vector<std::unique_ptr<Definition>>;

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

	class Identifier : public Expression {
	public:
		Identifier(const Token& idToken) : idToken{ idToken } {}

		void print() const override {
			std::cout << "name '" << idToken.value << "'";
		}

		TYPE(NodeType::Identifier)
	public:
		Token idToken;
	};

	// MISC

	class Argument {
	public:
		Argument(const std::string& name, ExpressionPtr expression) : name{ name }, expression{ std::move(expression) } {}
		void print() {
			std::cout << "arg " << name;
			expression->print();
		}
	public:
		std::string name;
		ExpressionPtr expression;
	};

	using ArgumentPtr = std::unique_ptr<Argument>;
	using ArgumentList = std::vector<ArgumentPtr>;


	class Call : public Expression {
	public:
		Call(const Token& funcToken, ArgumentList& argumentList) : funcToken{ funcToken }, argumentList{ std::move(argumentList) } {}

		void print() const override {
			std::cout << "call '" << funcToken.value << "' (";
			for (auto& arg : argumentList) {
				arg->print();
				std::cout << ", ";
			}
			std::cout << ")";
		}

		TYPE(NodeType::Call)
	public:
		Token funcToken;
		ArgumentList argumentList;
	};

	//
	// Statements
	//

	class NullStatement : public Statement {
	public:
		void print(uint8_t indent = 0) const override {
			std::cout << "->\n";
		}
		TYPE(NodeType::NullStatement)
	};

	class CompoundStatement : public Statement {
	public:
		CompoundStatement(StatementList& statementList) : statements{ std::move(statementList) } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "{\n";
			for (size_t i = 0; i < statements.size(); i++)
				statements[i]->print();
			std::cout << "}\n";
		}
		TYPE(NodeType::CompoundStatement)
	public:
		StatementList statements;
	};

	class IfStatement : public Statement {
	public:
		IfStatement(ExpressionPtr condition, StatementPtr ifBody, StatementPtr elseBody)
			: condition{ std::move(condition) }, ifBody{ std::move(ifBody) }, elseBody{ std::move(elseBody) } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "->  If ";
			condition->print();
			std::cout << " then ";
			ifBody->print();
			if (elseBody) {
				std::cout << "    else ";
				elseBody->print();
			}
		}
		TYPE(NodeType::IfStatement)
	public:
		ExpressionPtr condition;
		StatementPtr ifBody;
		StatementPtr elseBody;
	};

	class LoopStatement : public Statement {
	public:
		LoopStatement(StatementPtr loopBody) : loopBody{ std::move(loopBody) } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "->  Loop ";
			loopBody->print();
		}
		TYPE(NodeType::LoopStatement)
	public:
		StatementPtr loopBody;
	};

	class WhileStatement : public Statement {
	public:
		WhileStatement(ExpressionPtr condition, StatementPtr loopBody) : condition{ std::move(condition) }, loopBody{ std::move(loopBody) } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "->  While ";
			condition->print();
			std::cout << " do ";
			loopBody->print();
		}
		TYPE(NodeType::WhileStatement)
	public:
		ExpressionPtr condition;
		StatementPtr loopBody;
	};

	class BreakStatement : public Statement {
	public:
		BreakStatement(const Token& token) : token{ token } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "->  Break\n";
		}
		TYPE(NodeType::BreakStatement)
	public:
		Token token;
	};

	class ContinueStatement : public Statement {
	public:
		ContinueStatement(const Token& token) : token{ token } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "->  Continue\n";
		}
		TYPE(NodeType::ContinueStatement)
	public:
		Token token;
	};

	class ExpressionStatement : public Statement {
	public:
		ExpressionStatement(ExpressionPtr expression) : expression{ std::move(expression) } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "->  ";
			expression->print();
			std::cout << '\n';
		}
		TYPE(NodeType::ExpressionStatement)
	public:
		ExpressionPtr expression;
	};

	class ReturnStatement : public Statement {
	public:
		ReturnStatement(ExpressionPtr expression, const Token& token) : expression{ std::move(expression) }, token{ token } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "->  return ";
			expression->print();
			std::cout << '\n';
		}
		TYPE(NodeType::ReturnStatement)
	public:
		ExpressionPtr expression;
		Token token;
	};


	// MISC

	class Parameter {
	public:
		Parameter(const std::string& name, Type type, ExpressionPtr defaultValue) 
			: name{ name }, type{ type }, defaultValue { std::move(defaultValue) } {}
		void print() {
			std::cout << "parameter " << name;
			defaultValue->print();
		}
	public:
		std::string name;
		Type type;
		ExpressionPtr defaultValue;
	};

	using ParameterPtr = std::unique_ptr<Parameter>;
	using ParameterList = std::vector<ParameterPtr>;

	//
	// DEFINITIONS
	//

	class FunctionDefinition : public Definition {
	public:
		FunctionDefinition(const std::string& functionName, ParameterList& parameterList, Type returnType, StatementPtr functionBody)
			: functionBody{ std::move(functionBody) }, parameterList{ std::move(parameterList) }, returnType{ returnType }, functionName{ functionName } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "=>  function '" << functionName << "'(";
			for (auto& parameter : parameterList) {
				std::cout << parameter->name << ", ";
			}
			std::cout << "):\n";
			functionBody->print();
			std::cout << "!=> \n";
		}
		TYPE(NodeType::ReturnStatement)
	public:
		std::string functionName;
		Type returnType;
		ParameterList parameterList;
		StatementPtr functionBody;
	};

	class VariableDefinition : public Definition {
	public:
		VariableDefinition(const std::string& variableName, Type variableType, ExpressionPtr initializer, bool isConst, bool isGlobal)
			: initializer{ std::move(initializer) }, variableType{ variableType }, variableName{ variableName }, isConst{ isConst }, isGlobal{ isGlobal } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "=>  " << (isGlobal ? "global " : "local ") << (isConst ? "const " : "") << "variable '" << variableName << "': ";
			if (initializer) initializer->print();
			std::cout << '\n';
		}
		TYPE(NodeType::VariableDefinition)
	public:
		std::string variableName;
		Type variableType;
		ExpressionPtr initializer;
		bool isConst;
		bool isGlobal;
	};
}