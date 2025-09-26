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
	private:
		// -- Utilities --
		
		// Returns current token from the tokenlist
		const Token& currentToken() const;

		// Updates the index and current token
		void advance();
		const Token& peek(uint32_t depth = 1) const;

		// If current token matches the given type, advance, else throw error
		void consume(TokenType type, const std::string& errorMessage);

		// -- EXPRESSIONS --

		ExpressionPtr parseExpression(BindingPower bindingPower = 0);
		// Null Denotation
		ExpressionPtr nud(const Token& token);
		// Left Denotation
		ExpressionPtr led(const Token& token, ExpressionPtr left);

		// -- STATEMENTS --

		StatementPtr parseStatement();
		StatementPtr statementExpression();
	private:
		int32_t m_currentIndex{ -1 };
		Token* m_currentToken{ nullptr };
		TokenList m_tokenList{};

		StatementList m_statements{};

		ErrorHandler* m_errorHandler{ nullptr };
	};
}