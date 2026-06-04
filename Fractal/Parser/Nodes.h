// Nodes.h
// Contains the definitions of all Abstract Syntax Tree Nodes
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include "Common.h"
#include "Lexer/Token.h"
#include "Visitor.h"
#include <iostream>

namespace Fractal {

// Visitor double-dispatch boilerplate. Each concrete node ends up with one
// of these depending on which visitor hierarchy it belongs to.
#define EXPR_ACCEPT() void accept(ExpressionVisitor& v) override { v.visit(*this); }
#define STMT_ACCEPT() void accept(StatementVisitor& v) override { v.visit(*this); }

	enum class Decorator {
		None,
		External,
		Internal,
	};


	// Base Classes
	class Expression {
	public:
		virtual ~Expression() = default;
		virtual void accept(ExpressionVisitor& v) = 0;
		virtual void print() const {}
		TypePtr expressionType;
	};

	class Statement {
	public:
		virtual ~Statement() = default;
		virtual void accept(StatementVisitor& v) = 0;
		virtual void print(uint8_t indent = 0) const {}
	};

	// Derives from Statement in order to be able to create variables in local scope
	class Definition : public Statement {
	public:
		virtual ~Definition() = default;
		virtual void print(uint8_t indent = 0) const override {}
		virtual std::string getName() const { return ""; };
	};

	// AST nodes are uniquely owned: a parent expression owns its children, a
	// CompoundStatement owns its statements, etc. unique_ptr makes that
	// ownership explicit and removes the atomic refcount overhead of shared_ptr.
	using ExpressionPtr = std::unique_ptr<Expression>;
	using StatementPtr = std::unique_ptr<Statement>;
	using DefinitionPtr = std::unique_ptr<Definition>;

	using StatementList = std::vector<StatementPtr>;
	using DefinitionList = std::vector<DefinitionPtr>;

	//
	// Expressions
	//

	class IntegerLiteral : public Expression {
	public:
		IntegerLiteral(int64_t value, const Position& position) : value{ value }, position{ position } {}
		EXPR_ACCEPT()
		void print() const override { std::cout << value; }
	public:
		int64_t value;
		Size size{Size::DWord};
		Position position;
	};

	class FloatLiteral : public Expression {
	public:
		FloatLiteral(double value, const Position& position) : value{ value }, position{ position } {}
		EXPR_ACCEPT()
		void print() const override { std::cout << value; }
	public:
		double value;
		Size size{Size::DWord};
		Position position;
	};

	class StringLiteral : public Expression {
	public:
		StringLiteral(const std::string& value, const Position& position) : value{ value }, position{ position } {}
		EXPR_ACCEPT()
		void print() const override { std::cout << '"' << value << '"'; }
	public:
		std::string value;
		Position position;
	};

	class CharacterLiteral : public Expression {
	public:
		CharacterLiteral(std::string value, const Position& position) : value{ value }, position{ position } {}
		EXPR_ACCEPT()
		void print() const override { std::cout << "'" << value << "'"; }
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
		ArrayList(std::vector<ArrayElement> elements) : elements{ std::move(elements) } {}
		EXPR_ACCEPT()
		void print() const override {
			std::cout << "Array [";
			for (auto& element : elements) {
				element.expression->print();
				std::cout << ", ";
			}
			std::cout << ']';
		}
	public:
		std::vector<ArrayElement> elements;
		TypePtr elementType{ nullptr };
	};

	class UnaryOperation : public Expression {
	public:
		UnaryOperation(const Token& operatorToken, ExpressionPtr expression)
			: expression{ std::move(expression) }, operatorToken{ operatorToken } {}
		EXPR_ACCEPT()

		void print() const override {
			std::cout << operatorToken.value << '(';
			expression->print();
			std::cout << ')';
		}

	public:
		ExpressionPtr expression;
		Token operatorToken;
	};

	class BinaryOperation : public Expression {
	public:
		BinaryOperation(ExpressionPtr left, const Token& operatorToken, ExpressionPtr right)
			: left{ std::move(left) }, right{ std::move(right) }, operatorToken{ operatorToken } {}
		EXPR_ACCEPT()

		void print() const override {
			std::cout << '(';
			left->print();
			std::cout << ' ' << operatorToken.value << ' ';
			right->print();
			std::cout << ')';
		}

	public:
		ExpressionPtr left;
		ExpressionPtr right;
		Token operatorToken;
	};

	class Identifier : public Expression {
	public:
		Identifier(const Token& idToken) : idToken{ idToken } {}
		EXPR_ACCEPT()

		void print() const override {
			std::cout << "name '" << idToken.value << "'";
		}

	public:
		Token idToken;
	};

	class Assignment : public Expression {
	public:
		Assignment(ExpressionPtr left, const Token& operatorToken, ExpressionPtr right)
			: left{ std::move(left) }, right{ std::move(right) }, operatorToken{ operatorToken } {}
		EXPR_ACCEPT()

		void print() const override {
			std::cout << "(Assign ";
			left->print();
			std::cout << " = ";
			right->print();
			std::cout << ')';
		}

	public:
		ExpressionPtr left;
		ExpressionPtr right;
		Token operatorToken;
	};

	class MemberAccess : public Expression {
	public:
		MemberAccess(ExpressionPtr left, const Token& operatorToken, ExpressionPtr right)
			: left{ std::move(left) }, right{ std::move(right) }, operatorToken{ operatorToken } {}
		EXPR_ACCEPT()

		void print() const override {
			std::cout << "(Access ";
			right->print();
			std::cout << " from " << (operatorToken.type == DOT ? "" : "pointer ");
			left->print();
			std::cout << ')';
		}

	public:
		ExpressionPtr left;
		ExpressionPtr right;
		Token operatorToken;
	};

	class CastExpression : public Expression {
	public:
		CastExpression(ExpressionPtr expr, TypePtr target) : expr{ std::move(expr) }, target{ target } {}
		EXPR_ACCEPT()
		void print() const override {
			std::cout << "Cast ";
			expr->print();
			std::cout << " to "<< target->typeName();
		}
	public:
		ExpressionPtr expr;
		TypePtr target;
	};

	class DereferenceExpression : public Expression {
	public:
		DereferenceExpression(ExpressionPtr expression) : expr{ std::move(expression) } {}
		EXPR_ACCEPT()
		void print() const override {
			std::cout << "Dereference ";
			expr->print();
		}
	public:
		ExpressionPtr expr;
	};

	class AddressOfExpression : public Expression {
	public:
		AddressOfExpression(ExpressionPtr expression) : expr{ std::move(expression) } {}
		EXPR_ACCEPT()
		void print() const override {
			std::cout << "Address of ";
			expr->print();
		}
	public:
		ExpressionPtr expr;
	};

	// MISC

	class Argument {
	public:
		Argument(const std::string& name, ExpressionPtr expression)
			: name{ name }, expression{ std::move(expression) } {}
		void print() const {
			std::cout << "arg " << name;
			if (expression) expression->print();
		}
	public:
		std::string name;
		ExpressionPtr expression;
	};

	using ArgumentPtr = std::unique_ptr<Argument>;
	using ArgumentList = std::vector<ArgumentPtr>;


	class Call : public Expression {
	public:
		Call(const Token& funcToken, ArgumentList argumentList)
			: funcToken{ funcToken }, argumentList{ std::move(argumentList) } {}
		EXPR_ACCEPT()

		void print() const override {
			std::cout << "call '" << funcToken.value << "' (";
			for (auto& arg : argumentList) {
				arg->print();
				std::cout << ", ";
			}
			std::cout << ")";
		}

	public:
		Token funcToken;
		ArgumentList argumentList;
	};

	//
	// Statements
	//

	class NullStatement : public Statement {
	public:
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			(void)indent;
			std::cout << "->\n";
		}
	};

	class CompoundStatement : public Statement {
	public:
		CompoundStatement(StatementList statementList) : statements{ std::move(statementList) } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			(void)indent;
			std::cout << "{\n";
			for (size_t i = 0; i < statements.size(); i++)
				statements[i]->print();
			std::cout << "}\n";
		}
	public:
		StatementList statements;
	};

	class IfStatement : public Statement {
	public:
		IfStatement(ExpressionPtr condition, StatementPtr ifBody, StatementPtr elseBody)
			: condition{ std::move(condition) }, ifBody{ std::move(ifBody) }, elseBody{ std::move(elseBody) } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			(void)indent;
			std::cout << "->  If ";
			if (condition) condition->print();
			std::cout << " then ";
			if (ifBody) ifBody->print();
			if (elseBody) {
				std::cout << "    else ";
				elseBody->print();
			}
		}
	public:
		ExpressionPtr condition;
		StatementPtr ifBody;
		StatementPtr elseBody;
	};

	class LoopStatement : public Statement {
	public:
		LoopStatement(StatementPtr loopBody) : loopBody{ std::move(loopBody) } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			(void)indent;
			std::cout << "->  Loop ";
			if (loopBody) loopBody->print();
		}
	public:
		StatementPtr loopBody;
	};

	class WhileStatement : public Statement {
	public:
		WhileStatement(ExpressionPtr condition, StatementPtr loopBody)
			: condition{ std::move(condition) }, loopBody{ std::move(loopBody) } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			(void)indent;
			std::cout << "->  While ";
			if (condition) condition->print();
			std::cout << " do ";
			if (loopBody) loopBody->print();
		}
	public:
		ExpressionPtr condition;
		StatementPtr loopBody;
	};

	class BreakStatement : public Statement {
	public:
		BreakStatement(const Token& token) : token{ token } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			(void)indent;
			std::cout << "->  Break\n";
		}
	public:
		Token token;
		uint8_t loopIndex{ 0 };
	};

	class ContinueStatement : public Statement {
	public:
		ContinueStatement(const Token& token) : token{ token } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			(void)indent;
			std::cout << "->  Continue\n";
		}
	public:
		Token token;
		uint8_t loopIndex{ 0 };
	};

	class ExpressionStatement : public Statement {
	public:
		ExpressionStatement(ExpressionPtr expression, const Position& expressionPos)
			: expression{ std::move(expression) }, expressionPos{ expressionPos } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			(void)indent;
			std::cout << "->  ";
			if (expression) expression->print();
			std::cout << '\n';
		}
	public:
		ExpressionPtr expression;
		Position expressionPos;
	};

	class ReturnStatement : public Statement {
	public:
		ReturnStatement(ExpressionPtr expression, const Token& token)
			: expression{ std::move(expression) }, token{ token } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			(void)indent;
			std::cout << "->  return ";
			if (expression) expression->print();
			std::cout << '\n';
		}
	public:
		ExpressionPtr expression;
		Token token;
	};


	// MISC

	class Parameter {
	public:
		Parameter(const Token& nameToken, TypePtr type, ExpressionPtr defaultValue)
			: nameToken{ nameToken }, type{ type }, defaultValue{ std::move(defaultValue) } {}
		void print() const {
			std::cout << "parameter " << nameToken.value;
			if (defaultValue) defaultValue->print();
		}
	public:
		Token nameToken;
		TypePtr type;
		ExpressionPtr defaultValue;
	};

	using ParameterPtr = std::unique_ptr<Parameter>;
	using ParameterList = std::vector<ParameterPtr>;

	//
	// DEFINITIONS
	//

	class FunctionDefinition : public Definition {
	public:
		FunctionDefinition(const Token& nameToken, ParameterList parameterList, TypePtr returnType, StatementPtr functionBody)
			: nameToken{ nameToken }, returnType{ returnType },
			  parameterList{ std::move(parameterList) }, functionBody{ std::move(functionBody) } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			std::cout << "=>  function '" << nameToken.value << "'(";
			for (auto& parameter : parameterList) {
				std::cout << parameter->nameToken.value << ", ";
			}
			std::cout << "):\n";
			functionBody->print();
			std::cout << "!=> \n";
		}
		std::string getName() const override { return nameToken.value; }
	public:
		Token nameToken;
		TypePtr returnType;
		ParameterList parameterList;
		StatementPtr functionBody;
	};

	class VariableDefinition : public Definition {
	public:
		VariableDefinition(const Token& nameToken, TypePtr variableType, ExpressionPtr initializer, bool isConst, bool isGlobal)
			: nameToken{ nameToken }, variableType{ variableType },
			  initializer{ std::move(initializer) }, isConst{ isConst }, isGlobal{ isGlobal } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			std::cout << "=>  " << (isGlobal ? "global " : "local ") << (isConst ? "const " : "") << "variable '" << nameToken.value << "': ";
			if (initializer) initializer->print();
			std::cout << '\n';
		}
		std::string getName() const override { return nameToken.value; }
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
		ClassDefinition(const std::string& className, MemberList definitions)
			: className{ className }, definitions{ std::move(definitions) } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			std::cout << "=>  " << "Class '" << className << "': {\n";
			for (auto& [definition, decoration] : definitions) {
				std::cout << (decoration == ClassDecoration::Private ? "private " : "public ");
				definition->print();
			}
			std::cout << "}\n";
		}
	public:
		std::string className;
		MemberList definitions;
	};

	class DecoratedDefinition : public Definition {
	public:
		DecoratedDefinition(Decorator decorator, DefinitionPtr definition)
			: decorator{ decorator }, definition{ std::move(definition) } {}
		STMT_ACCEPT()
		void print(uint8_t indent = 0) const override {
			std::cout << "Decorated ";
			definition->print();
		}
		std::string getName() const override { return definition->getName(); }
	public:
		Decorator decorator;
		DefinitionPtr definition;
	};

	struct ProgramFile {
		DefinitionList definitions;

		// Statements that are run when 'filename'() function is called from another file.
		// In the case of the main project file, those statements act as the innards of the main() function that is called at execution
		StatementList statements;
	};
}