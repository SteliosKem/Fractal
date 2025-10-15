// Parser.cpp
// Contains the Parser Class method implementations
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "Parser.h"
#include "Lexer/Type.h"

namespace Fractal {
	uint8_t tokenBindingPower(const Token& token) {
		switch (token.type) {
			case DOT:
			case ARROW:
				return 110;
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
			case EQUAL:
				return 20;
			default:
				return 0;
		}
	}

	const StatementList& Parser::statements() const {
		return m_programFile.statements;
	}

	const DefinitionList& Parser::definitions() const {
		return m_programFile.definitions;
	}

	ProgramFile& Parser::program() {
		return m_programFile;
	}

	void Parser::advance() {
		m_currentIndex++;
		if (m_currentIndex < m_tokenList.size())
			m_currentToken = &m_tokenList[m_currentIndex];
	}

	void Parser::pushStatement(StatementPtr statement) {
		m_programFile.statements.push_back(statement);
	}

	void Parser::pushDefinition(DefinitionPtr definition) {
		m_programFile.definitions.push_back(definition);
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

	TypePtr Parser::parseType() {
		switch (currentToken().type) {
			case LEFT_PARENTHESIS: {
				advance();
				TypePtr type = parseType();
				consume(RIGHT_PARENTHESIS, "Expected ')'");
				return std::make_shared<PointerType>(type);
			}
			case LEFT_BRACKET: {
				advance();
				TypePtr type = parseType();
				consume(RIGHT_BRACKET, "Expected ']'");
				return std::make_shared<ArrayType>(type);
			}
			default: {
				if (!isTypeToken(currentToken())) {
					Token* typeToken = m_currentToken;
					advance();
					return std::make_shared<UserDefinedType>(typeToken->value);
				}
				BasicType type = getBasicType(currentToken());
				advance();
				return std::make_shared<FundamentalType>(type);
			}
		}
	}

	ExpressionPtr Parser::parseExpression(BindingPower bindingPower) {
		Token* token = m_currentToken;
		advance();
		ExpressionPtr left = nud(*token);
		while (tokenBindingPower(currentToken()) > bindingPower) {
			token = m_currentToken;
			advance();
			left = led(*token, left);
		}
		return left;
	}

	ExpressionPtr Parser::nud(const Token& token) {
		switch (token.type) {
		case TYPE_INTEGER:
		case TYPE_FLOAT:
		case STRING_LITERAL:
		case CHARACTER_LITERAL:
			return expressionLiteral(token);
		case MINUS:
		case BANG:
		case TILDE:
			return expressionUnary(token);
		case LEFT_PARENTHESIS:
			return expressionGroup(token);
		case IDENTIFIER:
			return expressionIdentifier(token);
		case LEFT_BRACKET:
			return expressionArray();
		default:
			m_errorHandler->reportError({ "Expected expression ", currentToken().position});
			return nullptr;
		}
	}

	ExpressionPtr Parser::expressionLiteral(const Token& token) {
		switch (token.type) {
		case TYPE_INTEGER:
			return std::make_shared<IntegerLiteral>(stoi(token.value), token.position);
		case TYPE_FLOAT:
			return std::make_shared<FloatLiteral>(stof(token.value), token.position);
		case STRING_LITERAL:
			return std::make_shared<StringLiteral>(token.value, token.position);
		case CHARACTER_LITERAL:
			return std::make_shared<CharacterLiteral>(token.value, token.position);
		default: return nullptr;
		}
		
	}

	ExpressionPtr Parser::expressionUnary(const Token& token) {
		ExpressionPtr expression = parseExpression(100); // Need high binding power
		return std::make_shared<UnaryOperation>(token, expression);
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
		return std::make_shared<Identifier>(token);
	}

	ExpressionPtr Parser::expressionCall(const Token& token) {
		advance();

		ArgumentList argList;

		// Parse arguments
		while (!match(RIGHT_PARENTHESIS) && !match(SPECIAL_EOF)) {
			argList.push_back(std::make_shared<Argument>("", parseExpression()));
			if (match(COMMA)) advance();
			else if (!match(RIGHT_PARENTHESIS)) break;
		}

		consume(RIGHT_PARENTHESIS, "Expected ')'");
		return std::make_shared<Call>(token, argList);
	}

	ExpressionPtr Parser::expressionArray() {
		std::vector<ArrayElement> expressions;
		while (!match(RIGHT_BRACKET) && !match(SPECIAL_EOF)) {
			Position first = currentToken().position;
			ExpressionPtr expr = parseExpression();
			Position second = first;
			second.endIndex = currentToken().position.endIndex;
			expressions.push_back({ expr, second});
			if (match(COMMA)) advance();
			else if (!match(RIGHT_BRACKET)) break;
		}

		consume(RIGHT_BRACKET, "Expected ']'");
		return std::make_shared<ArrayList>(expressions);
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
				return std::make_shared<BinaryOperation>(left, token, right);
			}
			case EQUAL: {
				BindingPower bindingPower = tokenBindingPower(token);
				ExpressionPtr right = parseExpression(bindingPower);
				return std::make_shared<Assignment>(left, token, right);
			}
			case ARROW:
			case DOT: {
				BindingPower bindingPower = tokenBindingPower(token);
				ExpressionPtr right = parseExpression(bindingPower);
				return std::make_shared<MemberAccess>(left, token, right);
			}
			default:
				return nullptr;
		}
	}

#define CONSUME_SEMICOLON() consume(SEMICOLON, "Expected ';'")
#define CONSUME_DOUBLE_ARROW() consume(DOUBLE_ARROW, "Expected '=>'")

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
			default: break;
		}
		return statementExpression();
	}

	StatementPtr Parser::statementNull() {
		advance();
		return std::make_shared<NullStatement>();
	}

	StatementPtr Parser::statementReturn() {
		Token* token = m_currentToken;
		advance();
		StatementPtr statement = std::make_shared<ReturnStatement>(parseExpression(), *token);
		CONSUME_SEMICOLON();
		return statement;
	}

	StatementPtr Parser::statementExpression() {
		Position pos = currentToken().position;
		ExpressionPtr expression = parseExpression();
		pos.endIndex = currentToken().position.endIndex;
		CONSUME_SEMICOLON();
		return std::make_shared<ExpressionStatement>(expression, pos);
	}

	StatementPtr Parser::statementCompound() {
		// Skip '{'
		advance();

		StatementList statements;

		while (!match(RIGHT_BRACE) && !match(SPECIAL_EOF))
			statements.push_back(parseStatement());

		consume(RIGHT_BRACE, "Expected '}'");
		return std::make_shared<CompoundStatement>(statements);
	}

	StatementPtr Parser::statementIf() {
		advance();
		ExpressionPtr condition = parseExpression();
		CONSUME_DOUBLE_ARROW();
		StatementPtr ifBody = parseStatement();
		StatementPtr elseBody = nullptr;
		if (match(ELSE)) {
			advance();
			elseBody = parseStatement();
		}
		return std::make_shared<IfStatement>(condition, ifBody, elseBody);
	}

	StatementPtr Parser::statementLoop() {
		advance();
		StatementPtr loopBody = parseStatement();
		return std::make_shared<LoopStatement>(loopBody);
	}

	StatementPtr Parser::statementWhile() {
		advance();
		ExpressionPtr condition = parseExpression();
		CONSUME_DOUBLE_ARROW();
		StatementPtr loopBody = parseStatement();
		return std::make_shared<WhileStatement>(condition, loopBody);
	}

	StatementPtr Parser::statementBreak() {
		Token* token = m_currentToken;
		advance();
		CONSUME_SEMICOLON();
		return std::make_shared<BreakStatement>(*token);
	}

	StatementPtr Parser::statementContinue() {
		Token* token = m_currentToken;
		advance();
		CONSUME_SEMICOLON();
		return std::make_shared<ContinueStatement>(*token);
	}

	// -- DEFINITIONS --

	void Parser::handleDefinitions() {
		// Skip '[define]'
		advance();
		advance();
		advance();

		DefinitionPtr ptr;
		while ((ptr = parseDefinition()) != nullptr)
			pushDefinition(ptr);

		if (!(currentToken().type == LESS && peek().type == BANG && peek(2).value == "define" && peek(3).type == GREATER))
			m_errorHandler->reportError({ "Expected end of definition set '<!define>'", currentToken().position });
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
		case CLASS:
			return definitionClass();
		default:
			return nullptr;
		}
	}

	DefinitionPtr Parser::definitionFunction() {
		// Skip 'fn'
		advance();

		TypePtr returnType = std::make_shared<FundamentalType>(BasicType::Null);

		Token* nameToken = m_currentToken;
		consume(IDENTIFIER, "Expected a function name after 'fn'");
		ParameterList parameterList;

		if (match(LEFT_PARENTHESIS)) {
			advance();
			while (!match(RIGHT_PARENTHESIS) && !match(SPECIAL_EOF)) {
				Token* paramToken = m_currentToken;
				consume(IDENTIFIER, "Expected parameter name");
				consume(COLON, "Expected ':' after parameter name to specify the parameter type");
				parameterList.push_back(std::make_shared<Parameter>(*paramToken, parseType(), nullptr));
				if (match(COMMA)) advance();
				else if (!match(RIGHT_PARENTHESIS)) break;
			}
			consume(RIGHT_PARENTHESIS, "Expected ')' after function arguments");
		}
		
		if (match(COLON)) {
			advance();
			returnType = parseType();
		}

		return std::make_shared<FunctionDefinition>(*nameToken, parameterList, returnType, parseStatement());
	}

	DefinitionPtr Parser::definitionVariable(bool isGlobal) {
		bool isConst = false;
		if (match(CONST))
			isConst = true;
		advance();

		TypePtr variableType = std::make_shared<FundamentalType>(BasicType::None);
		ExpressionPtr initializer = nullptr;
		Token* nameToken = m_currentToken;

		consume(IDENTIFIER, "Expected a variable name");

		bool specifiedType = false;
		if (match(COLON)) {
			specifiedType = true;
			advance();
			variableType = parseType();
		}

		if (match(EQUAL)) {
			advance();
			initializer = parseExpression();
		}
		else if (!specifiedType) m_errorHandler->reportError({ "A type must be specified if no initializer is given", currentToken().position});

		CONSUME_SEMICOLON();

		return std::make_shared<VariableDefinition>(*nameToken, variableType, initializer, isConst, isGlobal);
	}

	DefinitionPtr Parser::definitionClass() {
		advance();
		Token* nameToken = m_currentToken;
		consume(IDENTIFIER, "Expected class name");
		consume(LEFT_BRACE, "Expected '{' after class name");

		MemberList classMembers;

		while (!match(RIGHT_BRACE) && !match(SPECIAL_EOF)) {
			if (match(PUBLIC)) {
				advance();
				DefinitionPtr def = parseDefinition();
				if (def == nullptr)
					break;
				classMembers.push_back({ def, ClassDecoration::Public });
			}
			else if (match(PRIVATE)) {
				advance();
				DefinitionPtr def = parseDefinition();
				if (def == nullptr)
					break;
				classMembers.push_back({ def, ClassDecoration::Private });
			}
			else {
				m_errorHandler->reportError({ "Expected member definition", currentToken().position });
				advance();
			};
		}

		consume(RIGHT_BRACE, "Expected '}'");

		return std::make_shared<ClassDefinition>(nameToken->value, classMembers);
	}

	bool Parser::parse(const TokenList& tokens) {
		// Reset Values
		m_currentIndex = -1;
		m_tokenList = tokens;
		advance();
		while (currentToken().type != SPECIAL_EOF && !m_errorHandler->hasErrors()) {
			if (currentToken().type == LESS && peek().value == "define" && peek(2).type == GREATER) {
				handleDefinitions();
				continue;
			}
			pushStatement(parseStatement());
		}
		return !m_errorHandler->hasErrors();
	}
}