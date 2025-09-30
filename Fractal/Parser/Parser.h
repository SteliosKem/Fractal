// Parser.h
// Contains the Parser Class definition
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include "Lexer/Token.h"
#include "Nodes.h"
#include "Error/Error.h"

namespace Fractal {
	using BindingPower = uint8_t;
	BindingPower tokenBindingPower(const Token& token);

	class Parser {
	public:
		Parser(ErrorHandler* errorHandler) : m_errorHandler{ errorHandler } {}

		bool parse(const TokenList& tokens);
		const StatementList& statements() const;
		const DefinitionList& definitions() const;
		ProgramFile& program();
	private:
		// -- Utilities --
		
		// Returns current token from the tokenlist
		const Token& currentToken() const;

		// Updates the index and current token
		void advance();
		const Token& peek(uint32_t depth = 1) const;

		// Return if current token is of the given type
		bool match(TokenType type);

		// If current token matches the given type, advance, else throw error
		void consume(TokenType type, const std::string& errorMessage);

		// Is able to return basic types, pointers, arrays etc
		Type parseType();

		void pushStatement(StatementPtr statement);
		void pushDefinition(DefinitionPtr definition);

		// -- EXPRESSIONS --

		ExpressionPtr parseExpression(BindingPower bindingPower = 0);
		// Null Denotation
		ExpressionPtr nud(const Token& token);
		// Left Denotation
		ExpressionPtr led(const Token& token, ExpressionPtr left);
		ExpressionPtr expressionLiteral(const Token& token);
		ExpressionPtr expressionUnary(const Token& token);
		ExpressionPtr expressionGroup(const Token& token);
		ExpressionPtr expressionIdentifier(const Token& token);
		ExpressionPtr expressionCall(const Token& token);
		ExpressionPtr expressionArray();

		// -- STATEMENTS --

		StatementPtr parseStatement();
		StatementPtr statementNull();
		StatementPtr statementReturn();
		StatementPtr statementExpression();
		StatementPtr statementCompound();
		StatementPtr statementIf();
		StatementPtr statementWhile();
		StatementPtr statementLoop();
		StatementPtr statementBreak();
		StatementPtr statementContinue();

		// -- DEFINITIONS --

		void handleDefinitions();
		DefinitionPtr parseDefinition();
		DefinitionPtr definitionFunction();
		DefinitionPtr definitionVariable(bool isGlobal = true);
		DefinitionPtr definitionClass();
	private:
		int32_t m_currentIndex{ -1 };
		Token* m_currentToken{ nullptr };
		TokenList m_tokenList{};

		ProgramFile m_programFile{};

		ErrorHandler* m_errorHandler{ nullptr };
	};
}