// Parser.cpp
// Contains the Parser Class method implementations
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "Parser.h"

namespace Fractal {
	uint8_t tokenBindingPower(const Token& token) {
		switch (token.type) {
		case STAR:
		case SLASH:
			return 20;
		case PLUS:
		case MINUS:
			return 10;
		default:
			return 0;
		}
	}

	const StatementList& Parser::statements() const {
		return m_statements;
	}

	void Parser::advance() {
		m_currentIndex++;
		if (m_currentIndex < m_tokenList.size())
			m_currentToken = &m_tokenList[m_currentIndex];
	}

	void Parser::consume(TokenType type, const std::string& errorMessage) {
		if (currentToken().type == type)
			advance();
		else
			m_errorHandler->reportError({ errorMessage, currentToken().position });
	}

	const Token& Parser::currentToken() const {
		return *m_currentToken;
	}

	const Token& Parser::peek(uint32_t depth) const {
		if (m_currentIndex + depth < m_tokenList.size())
			return m_tokenList[m_currentIndex + depth];
		return m_tokenList[m_tokenList.size() - 1];
	}

	ExpressionPtr Parser::parseExpression(BindingPower bindingPower) {
		Token* token = m_currentToken;
		advance();
		ExpressionPtr left = nud(*token);
		while (tokenBindingPower(currentToken()) > bindingPower) {
			token = m_currentToken;
			advance();
			left = led(*token, std::move(left));
		}
		return left;
	}

	ExpressionPtr Parser::nud(const Token& token) {
		switch (token.type) {
		case TYPE_INTEGER:
			return std::make_unique<IntegerLiteral>(stoi(token.value), token.position);
		case MINUS: {
			ExpressionPtr expression = parseExpression(100); // Need high binding power
			return std::make_unique<UnaryOperation>(token, std::move(expression));
		}
		case LEFT_PARENTHESIS: {
			ExpressionPtr expression = parseExpression();
			if (currentToken().type != RIGHT_PARENTHESIS) {
				m_errorHandler->reportError({ "Expected ')'", currentToken().position });
				return nullptr;
			}
			// Skip ')'
			advance();
			return expression;
		}
		default:
			m_errorHandler->reportError({ "Expected expression ", currentToken().position});
			return nullptr;
		}
	}

	ExpressionPtr Parser::led(const Token& token, ExpressionPtr left) {
		switch (token.type) {
			case PLUS:
			case MINUS:
			case STAR: 
			case SLASH: {
				BindingPower bindingPower = tokenBindingPower(token);
				ExpressionPtr right = parseExpression(bindingPower);
				return std::make_unique<BinaryOperation>(std::move(left), token, std::move(right));
			}
			default:
				return nullptr;
		}
	}

#define CONSUME_SEMICOLON() consume(SEMICOLON, "Expected ';'")

	std::unique_ptr<Statement> Parser::parseStatement() {
		// Expression Statements for now
		return statementExpression();
	}

	StatementPtr Parser::statementExpression() {
		StatementPtr statement = std::make_unique<ExpressionStatement>(parseExpression());
		CONSUME_SEMICOLON();
		return statement;
	}

	bool Parser::parse(const TokenList& tokens) {
		// Reset Values
		m_currentIndex = -1;
		m_tokenList = tokens;
		advance();

		m_statements.push_back(parseStatement());
		return !m_errorHandler->hasErrors();
		return true;
	}
}