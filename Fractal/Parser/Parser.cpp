// Parser.cpp
// Contains the Parser Class method implementations
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "Parser.h"
#include "Lexer/Type.h"

namespace Fractal {
	uint8_t tokenBindingPower(const Token& token) {
		switch (token.type) {
			case STAR:
			case SLASH:
				return 80;
			case PLUS:
			case MINUS:
				return 70;
			case GREATER:
			case LESS:
			case GREATER_EQUAL:
			case LESS_EQUAL:
				return 60;
			case EQUAL_EQUAL:
			case BANG_EQUAL:
				return 50;
			case AND:
				return 40;
			case OR:
				return 30;
			default:
				return 0;
		}
	}

	const StatementList& Parser::statements() const {
		return m_statements;
	}

	const DefinitionList& Parser::definitions() const {
		return m_definitions;
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

	bool Parser::match(TokenType type) {
		return currentToken().type == type;
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
			return expressionLiteral(token);
		case MINUS:
			return expressionUnary(token);
		case LEFT_PARENTHESIS:
			return expressionGroup(token);
		case IDENTIFIER:
			return expressionIdentifier(token);
		default:
			m_errorHandler->reportError({ "Expected expression ", currentToken().position});
			return nullptr;
		}
	}

	ExpressionPtr Parser::expressionLiteral(const Token& token) {
		return std::make_unique<IntegerLiteral>(stoi(token.value), token.position);
	}

	ExpressionPtr Parser::expressionUnary(const Token& token) {
		ExpressionPtr expression = parseExpression(100); // Need high binding power
		return std::make_unique<UnaryOperation>(token, std::move(expression));
	}

	ExpressionPtr Parser::expressionGroup(const Token& token) {
		ExpressionPtr expression = parseExpression();
		if (currentToken().type != RIGHT_PARENTHESIS) {
			m_errorHandler->reportError({ "Expected ')'", currentToken().position });
			return nullptr;
		}
		// Skip ')'
		advance();
		return expression;
	}

	ExpressionPtr Parser::expressionIdentifier(const Token& token) {
		if (match(LEFT_PARENTHESIS))
			return expressionCall(token);
		return std::make_unique<Identifier>(token);
	}

	ExpressionPtr Parser::expressionCall(const Token& token) {
		advance();

		ArgumentList argList;

		// Parse arguments
		while (!match(RIGHT_PARENTHESIS) && !match(SPECIAL_EOF)) {
			argList.push_back(std::make_unique<Argument>("", std::move(parseExpression())));
			if (match(COMMA)) advance();
			else if (!match(RIGHT_PARENTHESIS)) break;
		}

		consume(RIGHT_PARENTHESIS, "Expected ')'");
		return std::make_unique<Call>(token, argList);
	}

	ExpressionPtr Parser::led(const Token& token, ExpressionPtr left) {
		switch (token.type) {
			case PLUS:
			case MINUS:
			case STAR: 
			case GREATER:
			case LESS:
			case EQUAL_EQUAL:
			case BANG_EQUAL:
			case GREATER_EQUAL:
			case LESS_EQUAL:
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
#define CONSUME_ARROW() consume(ARROW, "Expected '->'")

	StatementPtr Parser::parseStatement() {
		switch (currentToken().type) {
			case SEMICOLON:
				return statementNull();
			case LEFT_BRACE:
				return statementCompound();
			case RETURN:
				return statementReturn();
			case IF:
				return statementIf();
			case LOOP:
				return statementLoop();
			case WHILE:
				return statementWhile();
			case BREAK:
				return statementBreak();
			case CONTINUE:
				return statementContinue();
			case LET:
			case CONST:
				// Local variable
				return definitionVariable(false);
		}
		return statementExpression();
	}

	StatementPtr Parser::statementNull() {
		advance();
		return std::make_unique<NullStatement>();
	}

	StatementPtr Parser::statementReturn() {
		Token* token = m_currentToken;
		advance();
		StatementPtr statement = std::make_unique<ReturnStatement>(parseExpression(), *token);
		CONSUME_SEMICOLON();
		return statement;
	}

	StatementPtr Parser::statementExpression() {
		StatementPtr statement = std::make_unique<ExpressionStatement>(parseExpression());
		CONSUME_SEMICOLON();
		return statement;
	}

	StatementPtr Parser::statementCompound() {
		// Skip '{'
		advance();

		StatementList statements;

		while (!match(RIGHT_BRACE) && !match(SPECIAL_EOF))
			statements.push_back(parseStatement());

		consume(RIGHT_BRACE, "Expected '}'");
		return std::make_unique<CompoundStatement>(statements);
	}

	StatementPtr Parser::statementIf() {
		advance();
		ExpressionPtr condition = parseExpression();
		CONSUME_ARROW();
		StatementPtr ifBody = parseStatement();
		StatementPtr elseBody = nullptr;
		if (match(ELSE)) {
			advance();
			elseBody = parseStatement();
		}
		return std::make_unique<IfStatement>(std::move(condition), std::move(ifBody), std::move(elseBody));
	}

	StatementPtr Parser::statementLoop() {
		advance();
		StatementPtr loopBody = parseStatement();
		return std::make_unique<LoopStatement>(std::move(loopBody));
	}

	StatementPtr Parser::statementWhile() {
		advance();
		ExpressionPtr condition = parseExpression();
		CONSUME_ARROW();
		StatementPtr loopBody = parseStatement();
		return std::make_unique<WhileStatement>(std::move(condition), std::move(loopBody));
	}

	StatementPtr Parser::statementBreak() {
		Token* token = m_currentToken;
		advance();
		CONSUME_SEMICOLON();
		return std::make_unique<BreakStatement>(*token);
	}

	StatementPtr Parser::statementContinue() {
		Token* token = m_currentToken;
		advance();
		CONSUME_SEMICOLON();
		return std::make_unique<ContinueStatement>(*token);
	}

	// -- DEFINITIONS --

	void Parser::handleDefinitions() {
		// Skip '[define]'
		advance();
		advance();
		advance();

		DefinitionPtr ptr;
		while ((ptr = parseDefinition()) != nullptr)
			m_definitions.push_back(std::move(ptr));

		if (!(currentToken().type == LEFT_BRACKET && peek().type == BANG && peek(2).value == "define" && peek(3).value == "]"))
			m_errorHandler->reportError({ "Expected end of definition set '[!define]'", currentToken().position });
		else {
			advance();
			advance();
			advance();
			advance();
		}
	}

	DefinitionPtr Parser::parseDefinition() {
		switch (currentToken().type)
		{
		case FUNCTION:
			return definitionFunction();
		case LET:
		case CONST:
			return definitionVariable(true);
		default:
			return nullptr;
		}
	}

	DefinitionPtr Parser::definitionFunction() {
		// Skip 'fn'
		advance();

		Type returnType = Type::Null;

		Token* nameToken = m_currentToken;
		consume(IDENTIFIER, "Expected a function name after 'fn'");
		ParameterList parameterList;

		if (match(LEFT_PARENTHESIS)) {
			advance();
			while (!match(RIGHT_PARENTHESIS) && !match(SPECIAL_EOF)) {
				Token* nameToken = m_currentToken;
				consume(IDENTIFIER, "Expected parameter name");
				consume(COLON, "Expected ':' after parameter name to specify the parameter type");
				if (!isTypeToken(currentToken()))
					m_errorHandler->reportError({ "Expected type after ':'", currentToken().position });
				parameterList.push_back(std::make_unique<Parameter>(nameToken->value, getType(currentToken()), nullptr));
				advance();
				if (match(COMMA)) advance();
				else if (!match(RIGHT_PARENTHESIS)) break;
			}
			consume(RIGHT_PARENTHESIS, "Expected ')' after function arguments");
		}
		
		if (match(COLON)) {
			advance();
			Token* typeToken = m_currentToken;
			if (!isTypeToken(currentToken()))
				m_errorHandler->reportError({ "Expected a type after ':' in function definition", typeToken->position});
			advance();
		}

		return std::make_unique<FunctionDefinition>(nameToken->value, parameterList, returnType, std::move(parseStatement()));
	}

	DefinitionPtr Parser::definitionVariable(bool isGlobal) {
		bool isConst = false;
		if (match(CONST))
			isConst = true;
		advance();

		Type variableType = Type::Null;
		ExpressionPtr initializer = nullptr;
		Token* nameToken = m_currentToken;

		consume(IDENTIFIER, "Expected a variable name");

		bool specifiedType = false;
		if (match(COLON)) {
			specifiedType = true;
			advance();
			variableType = getType(currentToken());
			advance();
		}

		if (match(EQUAL)) {
			advance();
			initializer = parseExpression();
		}
		else if (!specifiedType) m_errorHandler->reportError({ "A type must be specified if no initializer is given", currentToken().position});

		CONSUME_SEMICOLON();

		return std::make_unique<VariableDefinition>(nameToken->value, variableType, std::move(initializer), isConst, isGlobal);
	}

	bool Parser::parse(const TokenList& tokens) {
		// Reset Values
		m_currentIndex = -1;
		m_tokenList = tokens;
		advance();
		while (currentToken().type != SPECIAL_EOF && !m_errorHandler->hasErrors()) {
			if (currentToken().type == LEFT_BRACKET && peek().value == "define" && peek(2).type == RIGHT_BRACKET) {
				handleDefinitions();
				continue;
			}
			m_statements.push_back(parseStatement());
		}
		return !m_errorHandler->hasErrors();
	}
}