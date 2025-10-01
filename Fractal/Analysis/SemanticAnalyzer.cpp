// SemanticAnalyzer.h
// Contains the SemanticAnalyzer Class member definitions
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "SemanticAnalyzer.h"

namespace Fractal {
	bool SemanticAnalyzer::findNameGlobal(const Token& nameToken) {
		if (m_globalTable.find(nameToken.value) != m_globalTable.end()) return true;
	}

	int32_t SemanticAnalyzer::findNameLocal(const Token& nameToken) {
		for (int32_t i = m_localStack.size() - 1; i >= 0; i--) {
			if (m_localStack[i].find(nameToken.value) != m_localStack[i].end()) return i;
		}
		return -1;
	}

	std::string SemanticAnalyzer::createUnique(const std::string& name) {
		static uint64_t index = 0;
		return name + "." + std::to_string(index++);
	}

	bool SemanticAnalyzer::analyze(ProgramFile* program) {
		m_program = program;
		m_localStack = {};
		m_globalTable = {};

		for (auto& definition : m_program->definitions)
			if (!analyzeDefinition(definition)) return false;

		pushScope();
		for (auto& statement : m_program->statements)
			if (!analyzeStatement(statement)) return false;
		popScope();

		return true;
	}

	bool SemanticAnalyzer::analyzeDefinition(DefinitionPtr definition) {
		switch (definition->getType()) {
			case NodeType::FunctionDefinition: return analyzeDefinitionFunction(definition);
			case NodeType::VariableDefinition: return analyzeDefinitionVariable(definition);
			case NodeType::ClassDefinition: return analyzeDefinitionClass(definition);
		}
	}

	void SemanticAnalyzer::pushScope() {
		m_localStack.push_back({});
	}

	void SemanticAnalyzer::popScope() {
		m_localStack.pop_back();
	}

	SymbolTable& SemanticAnalyzer::topScope() {
		return m_localStack[m_localStack.size() - 1];
	}

	bool SemanticAnalyzer::analyzeDefinitionFunction(DefinitionPtr definition) {
		std::shared_ptr<FunctionDefinition> functionDefinition = static_pointer_cast<FunctionDefinition>(definition);

		pushScope();

		if (findNameGlobal(functionDefinition->nameToken)) { 
			m_errorHandler->reportError({ "Function '" + functionDefinition->nameToken.value + "' is already defined", functionDefinition->nameToken.position });
			return false;
		}

		if (!analyzeParameters(functionDefinition->parameterList)) return false;

		std::vector<TypePtr> parameterTypes;
		for (auto param : functionDefinition->parameterList) {
			parameterTypes.push_back(param->type);
		}

		m_globalTable[functionDefinition->nameToken.value] = SymbolEntry{ functionDefinition->nameToken.value,
			std::make_shared<Type>(std::make_shared<FunctionType>(functionDefinition->returnType, parameterTypes), TypeInfo::Function)};

		if (!analyzeStatement(functionDefinition->functionBody)) return false;

		popScope();
		return true;
	}

	bool SemanticAnalyzer::analyzeParameters(const ParameterList& paramList) {
		std::vector<std::string> paramListCheck;
		for (const auto& parameter : paramList) {
			if (findNameGlobal(parameter->nameToken))
				m_errorHandler->reportWarning({ "Parameter '" + parameter->nameToken.value + "' shadows a global name", parameter->nameToken.position });
			if (std::find(paramListCheck.begin(), paramListCheck.end(), parameter->nameToken.value) != paramListCheck.end()) {
				m_errorHandler->reportError({ "Parameter '" + parameter->nameToken.value + "' is already defined", parameter->nameToken.position });
				return false;
			}
			paramListCheck.push_back(parameter->nameToken.value);
			topScope()[parameter->nameToken.value] = {createUnique(parameter->nameToken.value), parameter->type};
		}
		return true;
	}

	bool SemanticAnalyzer::analyzeDefinitionVariable(DefinitionPtr definition) { 
		std::shared_ptr<VariableDefinition> variableDefinition = static_pointer_cast<VariableDefinition>(definition);

		if (variableDefinition->isGlobal) {
			if (findNameGlobal(variableDefinition->nameToken)) {
				m_errorHandler->reportError({ "Variable '" + variableDefinition->nameToken.value + "' is already defined globally", variableDefinition->nameToken.position });
				return false;
			}

			m_globalTable[variableDefinition->nameToken.value] = SymbolEntry{ variableDefinition->nameToken.value, variableDefinition->variableType };
		}
		else {
			int32_t index = findNameLocal(variableDefinition->nameToken);
			if (index > -1) {
				m_errorHandler->reportError({ "Variable '" + variableDefinition->nameToken.value + "' is already defined in local scope", variableDefinition->nameToken.position });
				return false;
			}

			topScope()[variableDefinition->nameToken.value] = SymbolEntry{ variableDefinition->nameToken.value, variableDefinition->variableType };
		}

		if (variableDefinition->initializer && !analyzeExpression(variableDefinition->initializer)) return false;
	}

	bool SemanticAnalyzer::analyzeDefinitionClass(DefinitionPtr definition) { return true; }

	// -- STATEMENTS --
	bool SemanticAnalyzer::analyzeStatement(StatementPtr statement) {
		/*NullStatement,
		CompoundStatement,
		ExpressionStatement,
		ReturnStatement,
		IfStatement,
		LoopStatement,
		WhileStatement,
		BreakStatement,
		ContinueStatement,*/
		switch (statement->getType()) {
			case NodeType::CompoundStatement: return analyzeStatementCompound(statement);
			case NodeType::ExpressionStatement: return analyzeStatementExpression(statement);
			case NodeType::ReturnStatement: return analyzeStatementReturn(statement);
			case NodeType::LoopStatement: return analyzeStatementLoop(statement);
			case NodeType::IfStatement: return analyzeStatementIf(statement);
			case NodeType::WhileStatement: return analyzeStatementWhile(statement);
			case NodeType::BreakStatement: return analyzeStatementBreak(statement);
			case NodeType::ContinueStatement: return analyzeStatementContinue(statement);
			case NodeType::VariableDefinition: return analyzeDefinitionVariable(static_pointer_cast<Definition>(statement));
			case NodeType::NullStatement: return true;
		}
	}

	bool SemanticAnalyzer::analyzeStatementExpression(StatementPtr statement) { 
		std::shared_ptr<ExpressionStatement> expressionStatement = static_pointer_cast<ExpressionStatement>(statement);

		switch (expressionStatement->expression->getType()) {
		case NodeType::Call:
		case NodeType::MemberAccess:
		case NodeType::Assignment:
			break;
		default:
			m_errorHandler->reportWarning({ "Unused expression", expressionStatement->expressionPos });
		}

		return analyzeExpression(expressionStatement->expression);
	}

	bool SemanticAnalyzer::analyzeStatementCompound(StatementPtr statement) {
		pushScope();
		std::shared_ptr<CompoundStatement> compoundStatement = static_pointer_cast<CompoundStatement>(statement);

		for (auto ptr : compoundStatement->statements)
			if (!analyzeStatement(ptr)) return false;

		popScope();
		return true;
	}

	bool SemanticAnalyzer::analyzeStatementReturn(StatementPtr statement) {
		std::shared_ptr<ReturnStatement> returnStatement = static_pointer_cast<ReturnStatement>(statement);

		return analyzeExpression(returnStatement->expression);
	}

	bool SemanticAnalyzer::analyzeStatementIf(StatementPtr statement) {
		std::shared_ptr<IfStatement> ifStatement = static_pointer_cast<IfStatement>(statement);

		if (!analyzeExpression(ifStatement->condition)) return false;
		if (!analyzeStatement(ifStatement->ifBody)) return false;
		if (ifStatement->elseBody)
			if (!analyzeStatement(ifStatement->ifBody))
				return false;

		return true;
	}

	bool SemanticAnalyzer::analyzeStatementWhile(StatementPtr statement) {
		std::shared_ptr<WhileStatement> whileStatement = static_pointer_cast<WhileStatement>(statement);

		if (!analyzeExpression(whileStatement->condition)) return false;
		if (!analyzeStatement(whileStatement->loopBody)) return false;

		return true;
	}

	bool SemanticAnalyzer::analyzeStatementLoop(StatementPtr statement) {
		std::shared_ptr<LoopStatement> loopStatement = static_pointer_cast<LoopStatement>(statement);

		return analyzeStatement(loopStatement->loopBody);
	}

	bool SemanticAnalyzer::analyzeStatementBreak(StatementPtr statement) {
		return true;
	}

	bool SemanticAnalyzer::analyzeStatementContinue(StatementPtr statement) {
		return true;
	}

	// Expressions
	bool SemanticAnalyzer::analyzeExpression(ExpressionPtr expression) { 
		switch (expression->getType()) {
			case NodeType::IntegerLiteral: return analyzeExpressionInteger(expression);
			case NodeType::StringLiteral: return analyzeExpressionString(expression);
			case NodeType::CharacterLiteral: return analyzeExpressionCharacter(expression);
			case NodeType::FloatLiteral: return analyzeExpressionFloat(expression);
			case NodeType::ArrayList: return analyzeExpressionArray(expression);
			case NodeType::BinaryOperation: return analyzeExpressionBinary(expression);
			case NodeType::UnaryOperation: return analyzeExpressionUnary(expression);
			case NodeType::Identifier: return analyzeExpressionIdentifier(expression);
			case NodeType::Call: return analyzeExpressionCall(expression);
			case NodeType::Assignment: return analyzeExpressionAssignment(expression);
			case NodeType::MemberAccess: return analyzeExpressionMemberAccess(expression);
		}
	}

	bool SemanticAnalyzer::analyzeExpressionInteger(ExpressionPtr expression) {
		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionString(ExpressionPtr expression) {
		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionCharacter(ExpressionPtr expression) {
		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionFloat(ExpressionPtr expression) {
		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionArray(ExpressionPtr expression) {
		std::shared_ptr<ArrayList> arraylist = static_pointer_cast<ArrayList>(expression);

		for (auto ptr : arraylist->elements)
			if(!analyzeExpression(ptr)) return false;
	}

	bool SemanticAnalyzer::analyzeExpressionBinary(ExpressionPtr expression) {
		std::shared_ptr<BinaryOperation> binary = static_pointer_cast<BinaryOperation>(expression);

		if (!analyzeExpression(binary->left) || !analyzeExpression(binary->right)) return false;
	}

	bool SemanticAnalyzer::analyzeExpressionUnary(ExpressionPtr expression) {
		std::shared_ptr<UnaryOperation> unary = static_pointer_cast<UnaryOperation>(expression);

		return analyzeExpression(unary->expression);
	}

	bool SemanticAnalyzer::analyzeExpressionIdentifier(ExpressionPtr expression) {
		std::shared_ptr<Identifier> identifier = static_pointer_cast<Identifier>(expression);

		int32_t index = findNameLocal(identifier->idToken);
		if (index > -1) {
			identifier->idToken.value = m_localStack[index][identifier->idToken.value].name;
			return true;
		}
		if (!findNameGlobal(identifier->idToken)) {
			m_errorHandler->reportError({ "Undefined name '" + identifier->idToken.value + "'", identifier->idToken.position});
			return false;
		}

		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionCall(ExpressionPtr expression) { 
		std::shared_ptr<Call> call = static_pointer_cast<Call>(expression);

		int32_t index = findNameLocal(call->funcToken);
		if (index > -1)
			call->funcToken.value = m_localStack[index][call->funcToken.value].name;
		else if (!findNameGlobal(call->funcToken)) {
			m_errorHandler->reportError({ "Undefined name '" + call->funcToken.value + "'", call->funcToken.position });
			return false;
		}
		for (auto arg : call->argumentList)
			if (!analyzeExpression(arg->expression)) return false;

		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionAssignment(ExpressionPtr expression) { 
		std::shared_ptr<Assignment> assignment = static_pointer_cast<Assignment>(expression);

		switch (assignment->left->getType()) {
			case NodeType::Call:
			case NodeType::Identifier:
			case NodeType::MemberAccess:
				break;
			default:
				m_errorHandler->reportError({ "Cannot assign to non-lvalues", assignment->operatorToken.position});
				return false;
		}

		if (!analyzeExpression(assignment->left) || !analyzeExpression(assignment->right)) return false;

		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionMemberAccess(ExpressionPtr expression) { 
		std::shared_ptr<MemberAccess> access = static_pointer_cast<MemberAccess>(expression);
		switch (access->left->getType()) {
			case NodeType::Call:
			case NodeType::Identifier:
			case NodeType::MemberAccess:
				break;
			default:
				m_errorHandler->reportError({ "Cannot access member of non-lvalues", access->operatorToken.position });
				return false;
		}

		switch (access->right->getType()) {
		case NodeType::Call:
		case NodeType::Identifier:
		case NodeType::MemberAccess:
			break;
		default:
			m_errorHandler->reportError({ "Non-lvalues are not valid members of lvalues", access->operatorToken.position });
			return false;
		}
		if (access->left->getType() == NodeType::Identifier && !analyzeExpression(access->left)) return false;
		return true;
	}
}