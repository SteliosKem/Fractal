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
		StringLiteral,
		CharacterLiteral,
		FloatLiteral,
		ArrayList,
		BinaryOperation,
		UnaryOperation,
		Identifier,
		Call,
		Assignment,
		MemberAccess,

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
		VariableDefinition,
		ClassDefinition
	};

#define _TYPE(x) virtual NodeType getType() const { return x; }
#define TYPE(x) virtual NodeType getType() const override { return x; }

	// Base Classes
	class Expression {
	public:
		virtual ~Expression() = default;
		virtual void print() const {}
		TypePtr expressionType;
		_TYPE(NodeType::Expression)
	};

	class Statement {
	public:
		virtual ~Statement() = default;
		virtual void print(uint8_t indent = 0) const {}
		_TYPE(NodeType::Statement)
	};

	// Derives from Statement in order to be able to create variables in local scope
	class Definition : public Statement {
	public:
		virtual ~Definition() = default;
		virtual void print(uint8_t indent = 0) const override {}
		TYPE(NodeType::Definition)
	};

	using ExpressionPtr = std::shared_ptr<Expression>;
	using StatementPtr = std::shared_ptr<Statement>;
	using DefinitionPtr = std::shared_ptr<Definition>;

	using StatementList = std::vector<std::shared_ptr<Statement>>;
	using DefinitionList = std::vector<std::shared_ptr<Definition>>;

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

	class FloatLiteral : public Expression {
	public:
		FloatLiteral(double value, const Position& position) : value{ value }, position{ position } {}
		void print() const override { std::cout << value; }
		TYPE(NodeType::FloatLiteral)
	public:
		double value;
		Position position;
	};

	class StringLiteral : public Expression {
	public:
		StringLiteral(const std::string& value, const Position& position) : value{ value }, position{ position } {}
		void print() const override { std::cout << '"' << value << '"'; }
		TYPE(NodeType::StringLiteral)
	public:
		std::string value;
		Position position;
	};

	class CharacterLiteral : public Expression {
	public:
		CharacterLiteral(std::string value, const Position& position) : value{ value }, position{ position } {}
		void print() const override { std::cout << "'" << value << "'"; }
		TYPE(NodeType::CharacterLiteral)
	public:
		std::string value;
		Position position;
	};

	struct ArrayElement {
		ExpressionPtr expression;
		Position pos;
	};

	class ArrayList : public Expression {
	public:
		ArrayList(const std::vector<ArrayElement>& elements) : elements{ elements } {}
		void print() const override { 
			std::cout << "Array [";
			for (auto& element : elements) {
				element.expression->print();
				std::cout << ", ";
			}
			std::cout << ']';
		}
		TYPE(NodeType::ArrayList)
	public:
		std::vector<ArrayElement> elements;
		TypePtr elementType{ nullptr };
	};

	class UnaryOperation : public Expression {
	public:
		UnaryOperation(const Token& operatorToken, ExpressionPtr expression) : operatorToken{ operatorToken }, expression{ expression } {}

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
			: left{ left }, operatorToken{operatorToken}, right{ right } {}

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

	class Assignment : public Expression {
	public:
		Assignment(ExpressionPtr left, const Token& operatorToken, ExpressionPtr right)
			: left{ left }, operatorToken{ operatorToken }, right{ right } {}

		void print() const override {
			std::cout << "(Assign ";
			left->print();
			std::cout << " = ";
			right->print();
			std::cout << ')';
		}

		TYPE(NodeType::Assignment)
	public:
		ExpressionPtr left;
		ExpressionPtr right;
		Token operatorToken;
	};

	class MemberAccess : public Expression {
	public:
		MemberAccess(ExpressionPtr left, const Token& operatorToken, ExpressionPtr right)
			: left{ left }, operatorToken{ operatorToken }, right{ right } {}

		void print() const override {
			std::cout << "(Access ";
			right->print();
			std::cout << " from " << (operatorToken.type == DOT ? "" : "pointer ");
			left->print();
			std::cout << ')';
		}

		TYPE(NodeType::MemberAccess)
	public:
		ExpressionPtr left;
		ExpressionPtr right;
		Token operatorToken;
	};

	// MISC

	class Argument {
	public:
		Argument(const std::string& name, ExpressionPtr expression) : name{ name }, expression{ expression } {}
		void print() {
			std::cout << "arg " << name;
			expression->print();
		}
	public:
		std::string name;
		ExpressionPtr expression;
	};

	using ArgumentPtr = std::shared_ptr<Argument>;
	using ArgumentList = std::vector<ArgumentPtr>;


	class Call : public Expression {
	public:
		Call(const Token& funcToken, ArgumentList& argumentList) : funcToken{ funcToken }, argumentList{ argumentList } {}

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
		CompoundStatement(StatementList& statementList) : statements{ statementList } {}
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
			: condition{ condition }, ifBody{ ifBody }, elseBody{ elseBody } {}
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
		LoopStatement(StatementPtr loopBody) : loopBody{ loopBody } {}
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
		WhileStatement(ExpressionPtr condition, StatementPtr loopBody) : condition{ condition }, loopBody{ loopBody } {}
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
		uint8_t loopIndex{ 0 };
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
		uint8_t loopIndex{ 0 };
	};

	class ExpressionStatement : public Statement {
	public:
		ExpressionStatement(ExpressionPtr expression, const Position& expressionPos)
			: expression{ expression }, expressionPos{ expressionPos } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "->  ";
			expression->print();
			std::cout << '\n';
		}
		TYPE(NodeType::ExpressionStatement)
	public:
		ExpressionPtr expression;
		Position expressionPos;
	};

	class ReturnStatement : public Statement {
	public:
		ReturnStatement(ExpressionPtr expression, const Token& token) : expression{ expression }, token{ token } {}
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
		Parameter(const Token& nameToken, TypePtr type, ExpressionPtr defaultValue)
			: nameToken{ nameToken }, type{ type }, defaultValue { defaultValue } {}
		void print() {
			std::cout << "parameter " << nameToken.value;
			defaultValue->print();
		}
	public:
		Token nameToken;
		TypePtr type;
		ExpressionPtr defaultValue;
	};

	using ParameterPtr = std::shared_ptr<Parameter>;
	using ParameterList = std::vector<ParameterPtr>;

	//
	// DEFINITIONS
	//

	class FunctionDefinition : public Definition {
	public:
		FunctionDefinition(const Token& nameToken, ParameterList& parameterList, TypePtr returnType, StatementPtr functionBody)
			: functionBody{ functionBody }, parameterList{ parameterList }, returnType{ returnType }, nameToken{ nameToken } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "=>  function '" << nameToken.value << "'(";
			for (auto& parameter : parameterList) {
				std::cout << parameter->nameToken.value << ", ";
			}
			std::cout << "):\n";
			functionBody->print();
			std::cout << "!=> \n";
		}
		TYPE(NodeType::FunctionDefinition)
	public:
		Token nameToken;
		TypePtr returnType;
		ParameterList parameterList;
		StatementPtr functionBody;
	};

	class VariableDefinition : public Definition {
	public:
		VariableDefinition(const Token& nameToken, TypePtr variableType, ExpressionPtr initializer, bool isConst, bool isGlobal)
			: initializer{ initializer }, variableType{ variableType }, nameToken{ nameToken }, isConst{ isConst }, isGlobal{ isGlobal } {}
		void print(uint8_t indent = 0) const override {
			std::cout << "=>  " << (isGlobal ? "global " : "local ") << (isConst ? "const " : "") << "variable '" << nameToken.value << "': ";
			if (initializer) initializer->print();
			std::cout << '\n';
		}
		TYPE(NodeType::VariableDefinition)
	public:
		Token nameToken;
		TypePtr variableType;
		ExpressionPtr initializer;
		bool isConst;
		bool isGlobal;
	};

	enum class ClassDecoration {
		Public,
		Private,
	};

	using Member = std::pair<DefinitionPtr, ClassDecoration>;
	using MemberList = std::vector<Member>;

	class ClassDefinition : public Definition {
	public:
		ClassDefinition(const std::string& className, MemberList& definitions) : definitions{ definitions }, className{ className }{}
		void print(uint8_t indent = 0) const override {
			std::cout << "=>  " << "Class '" << className << "': {\n";
			for (auto& [definition, decoration] : definitions) {
				std::cout << (decoration == ClassDecoration::Private ? "private " : "public ");
				definition->print();
			}
			std::cout << "}\n";
		}
		TYPE(NodeType::ClassDefinition)
	public:
		std::string className;
		MemberList definitions;
	};

	struct ProgramFile {
		DefinitionList definitions;

		// Statements that are run when 'filename'() function is called from another file.
		// In the case of the main project file, those statements act as the innards of the main() function that is called at execution
		StatementList statements;
	};
}