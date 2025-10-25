// SemanticAnalyzer.h
// Contains the SemanticAnalyzer Class member definitions
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "SemanticAnalyzer.h"

namespace Fractal {
	bool SemanticAnalyzer::findNameGlobal(const Token& nameToken) {
		if (m_globalTable.find(nameToken.value) != m_globalTable.end()) return true;
		return false;
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

	uint8_t uniqueLoop() {
		static uint8_t index = 0;
		return index++;
	}

	bool SemanticAnalyzer::analyze(ProgramFile* program) {
		m_program = program;
		m_localStack = {};

		for (auto& definition : m_program->definitions)
			if (!analyzeDefinition(definition)) return false;

		pushScope();
		for (auto& statement : m_program->statements)
			if (!analyzeStatement(statement)) return false;
		popScope();

		return true;
	}

	bool SemanticAnalyzer::saveDefinitions(ProgramFile* program) {
		m_program = program;

		for(auto definition : m_program->definitions)
			if(!analyzeDefinition(definition)) return false;
	}

	bool SemanticAnalyzer::analyzeDefinition(DefinitionPtr definition, bool toSave) {
		switch (definition->getType()) {
			case NodeType::FunctionDefinition: return analyzeDefinitionFunction(definition, toSave);
			case NodeType::VariableDefinition: return analyzeDefinitionVariable(definition, toSave);
			case NodeType::ClassDefinition: return analyzeDefinitionClass(definition, toSave);
			default:
				return false;
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

	bool SemanticAnalyzer::analyzeDefinitionFunction(DefinitionPtr definition, bool toSave) {
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

		m_currentFunction = std::make_shared<FunctionType>(functionDefinition->returnType, parameterTypes);
		m_globalTable[functionDefinition->nameToken.value] = SymbolEntry{ functionDefinition->nameToken.value, m_currentFunction };
		
		if (!analyzeStatement(functionDefinition->functionBody)) return false;

		m_currentFunction = nullptr;
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
			std::string newName = createUnique(parameter->nameToken.value);
			topScope()[parameter->nameToken.value] = { newName, parameter->type };
			parameter->nameToken.value = newName;
		}
		return true;
	}

	bool SemanticAnalyzer::analyzeDefinitionVariable(DefinitionPtr definition, bool toSave) { 
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

		if (variableDefinition->initializer) {
			if (!analyzeExpression(variableDefinition->initializer)) return false;
			if (variableDefinition->variableType->typeInfo() == TypeInfo::Fundamental
				&& static_pointer_cast<FundamentalType>(variableDefinition->variableType)->type == BasicType::None)
				variableDefinition->variableType = variableDefinition->initializer->expressionType;
			else {
				if (!sameType(variableDefinition->initializer->expressionType, variableDefinition->variableType)) {
					m_errorHandler->reportError({ "Initializer Expression does not match the variable's type", variableDefinition->nameToken.position });
					return false;
				}
			}
		}

		return true;
	}

	bool SemanticAnalyzer::analyzeDefinitionClass(DefinitionPtr definition, bool toSave) { 
		std::shared_ptr<ClassDefinition> classDef = static_pointer_cast<ClassDefinition>(definition);
		m_userDefinedTypes.push_back(classDef->className);
		return true;
	}

	// -- STATEMENTS --
	bool SemanticAnalyzer::analyzeStatement(StatementPtr statement) {
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
			default: return false;
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
		if (!m_currentFunction) {
			m_errorHandler->reportError({"Cannot use return outside of a function body", returnStatement->token.position});
			return false;
		}

		if (!analyzeExpression(returnStatement->expression)) return false;
		if (!sameType(returnStatement->expression->expressionType, m_currentFunction->returnType)) {
			m_errorHandler->reportError({"Cannot return type '" + returnStatement->expression->expressionType->typeName() 
				+ "' from a function which returns type '" + m_currentFunction->returnType->typeName() + "'", returnStatement->token.position});
			return false;
		}
		return true;
	}

	bool SemanticAnalyzer::analyzeStatementIf(StatementPtr statement) {
		std::shared_ptr<IfStatement> ifStatement = static_pointer_cast<IfStatement>(statement);

		if (!analyzeExpression(ifStatement->condition)) return false;
		if (!analyzeStatement(ifStatement->ifBody)) return false;
		if (ifStatement->elseBody)
			if (!analyzeStatement(ifStatement->elseBody))
				return false;

		return true;
	}

	bool SemanticAnalyzer::analyzeStatementWhile(StatementPtr statement) {
		std::shared_ptr<WhileStatement> whileStatement = static_pointer_cast<WhileStatement>(statement);

		m_loopStack.push_back(uniqueLoop());

		if (!analyzeExpression(whileStatement->condition)) return false;
		if (!analyzeStatement(whileStatement->loopBody)) return false;

		m_loopStack.pop_back();

		return true;
	}

	bool SemanticAnalyzer::analyzeStatementLoop(StatementPtr statement) {
		std::shared_ptr<LoopStatement> loopStatement = static_pointer_cast<LoopStatement>(statement);
		m_loopStack.push_back(uniqueLoop());

		if (!analyzeStatement(loopStatement->loopBody)) return false;

		m_loopStack.pop_back();

		return true;
	}

	bool SemanticAnalyzer::analyzeStatementBreak(StatementPtr statement) {
		std::shared_ptr<BreakStatement> breakStatement = static_pointer_cast<BreakStatement>(statement);
		if (m_loopStack.size() == 0) {
			m_errorHandler->reportError({"Cannot use break outside of a loop", breakStatement->token.position});
			return false;
		}
		breakStatement->loopIndex = m_loopStack[m_loopStack.size() - 1];
		return true;
	}

	bool SemanticAnalyzer::analyzeStatementContinue(StatementPtr statement) {
		std::shared_ptr<ContinueStatement> continueStatement = static_pointer_cast<ContinueStatement>(statement);
		if (m_loopStack.size() == 0) {
			m_errorHandler->reportError({ "Cannot use continue outside of a loop", continueStatement->token.position });
			return false;
		}
		continueStatement->loopIndex = m_loopStack[m_loopStack.size() - 1];
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
			default: return false;
		}
	}

	bool SemanticAnalyzer::analyzeExpressionInteger(ExpressionPtr expression) {
		expression->expressionType = std::make_shared<FundamentalType>(BasicType::I32);
		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionString(ExpressionPtr expression) {
		expression->expressionType = std::make_shared<FundamentalType>(BasicType::String);
		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionCharacter(ExpressionPtr expression) {
		expression->expressionType = std::make_shared<FundamentalType>(BasicType::Character);
		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionFloat(ExpressionPtr expression) {
		expression->expressionType = std::make_shared<FundamentalType>(BasicType::F32);
		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionArray(ExpressionPtr expression) {
		std::shared_ptr<ArrayList> arraylist = static_pointer_cast<ArrayList>(expression);

		TypePtr firstType;

		if (arraylist->elements.size() > 0) {
			ArrayElement first = arraylist->elements[0];
			if (!analyzeExpression(first.expression)) return false;
			firstType = first.expression->expressionType;
		}

		for (size_t i = 1; i < arraylist->elements.size(); i++) {
			if (!analyzeExpression(arraylist->elements[i].expression)) return false;
			if (!sameType(arraylist->elements[i].expression->expressionType, firstType)) {
				// Have to add element position for errors
				m_errorHandler->reportError({ "Cannot insert element of type '" + arraylist->elements[i].expression->expressionType->typeName()
					+ "' to array which holds elements of type '" + firstType->typeName() + "'", arraylist->elements[i].pos });
				return false;
			}
		}

		arraylist->expressionType = std::make_shared<ArrayType>(firstType);
		arraylist->elementType = firstType;
		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionBinary(ExpressionPtr expression) {
		std::shared_ptr<BinaryOperation> binary = static_pointer_cast<BinaryOperation>(expression);

		if (!analyzeExpression(binary->left) || !analyzeExpression(binary->right)) return false;
		if (!sameType(binary->left->expressionType, binary->right->expressionType)) {
			m_errorHandler->reportError({ "Cannot operate between '" + binary->right->expressionType->typeName()
				+ "' and '" + binary->left->expressionType->typeName() + "' types", binary->operatorToken.position});
			return false;
		}

		binary->expressionType = binary->left->expressionType;
		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionUnary(ExpressionPtr expression) {
		std::shared_ptr<UnaryOperation> unary = static_pointer_cast<UnaryOperation>(expression);
		if (!analyzeExpression(unary->expression)) return false;
		unary->expressionType = unary->expression->expressionType;

		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionIdentifier(ExpressionPtr expression) {
		std::shared_ptr<Identifier> identifier = static_pointer_cast<Identifier>(expression);

		int32_t index = findNameLocal(identifier->idToken);
		if (index > -1) {
			identifier->expressionType = m_localStack[index][identifier->idToken.value].type;
			identifier->idToken.value = m_localStack[index][identifier->idToken.value].name;
			return true;
		}

		if (!findNameGlobal(identifier->idToken)) {
			m_errorHandler->reportError({ "Undefined name '" + identifier->idToken.value + "'", identifier->idToken.position});
			return false;
		}
		identifier->expressionType = m_globalTable[identifier->idToken.value].type;

		return true;
	}

	bool SemanticAnalyzer::compareArgsToParams(const std::vector<TypePtr>& paramList, std::shared_ptr<Call> call) {
		const ArgumentList& argList = call->argumentList;
		if (paramList.size() != argList.size()) {
			m_errorHandler->reportError(
				{"Expected " + std::to_string(paramList.size()) + " arguments in '" + call->funcToken.value + "' call, but got " + std::to_string(argList.size())
				, call->funcToken.position});
			return false;
		}
		for (size_t i = 0; i < paramList.size(); i++) {
			if (!sameType(paramList[i], argList[i]->expression->expressionType)) {
				m_errorHandler->reportError({ "Expected argument type '" + paramList[i]->typeName() 
					+ "', got '" + argList[i]->expression->expressionType->typeName(), call->funcToken.position});
				return false;
			}
		}
		return true;
	}

	bool SemanticAnalyzer::analyzeExpressionCall(ExpressionPtr expression) { 
		std::shared_ptr<Call> call = static_pointer_cast<Call>(expression);

		TypePtr type;

		std::shared_ptr<FunctionType> funcType;

		int32_t index = findNameLocal(call->funcToken);
		if (index > -1) {
			SymbolEntry& symbol = m_localStack[index][call->funcToken.value];
			funcType = static_pointer_cast<FunctionType>(symbol.type);
			if (symbol.type->typeInfo() != TypeInfo::Function) {
				m_errorHandler->reportError({ "Cannot call non-function names", call->funcToken.position});
				return false;
			}
			call->funcToken.value = symbol.name;
			call->expressionType = funcType->returnType;
		}
		else if (!findNameGlobal(call->funcToken)) {
			m_errorHandler->reportError({ "Undefined name '" + call->funcToken.value + "'", call->funcToken.position });
			return false;
		}
		else {
			SymbolEntry& symbol = m_globalTable[call->funcToken.value];
			funcType = static_pointer_cast<FunctionType>(symbol.type);
			if (symbol.type->typeInfo() != TypeInfo::Function) {
				m_errorHandler->reportError({ "Cannot call non-function names", call->funcToken.position });
				return false;
			}
			call->expressionType = static_pointer_cast<FunctionType>(symbol.type)->returnType;
		}

		for (auto arg : call->argumentList)
			if (!analyzeExpression(arg->expression)) return false;

		return compareArgsToParams(funcType->parameterTypes, call);
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
		if (!sameType(assignment->left->expressionType, assignment->right->expressionType)) {
			m_errorHandler->reportError({ "Cannot assign expression of type '" + assignment->right->expressionType->typeName()
				+ "' to variable of type '" + assignment->left->expressionType->typeName() + "'", assignment->operatorToken.position});
			return false;
		}

		assignment->expressionType = assignment->left->expressionType;

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