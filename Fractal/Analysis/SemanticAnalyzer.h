// SemanticAnalyzer.h
// Contains the SemanticAnalyzer Class definition
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once

#include "Error/Error.h"
#include "Parser/Nodes.h"

namespace Fractal {
	struct SymbolEntry {
		std::string name;
		TypePtr type;
	};
	using SymbolTable = std::unordered_map<std::string, SymbolEntry>;
	using GlobalNames = std::vector<std::string>;

	class SemanticAnalyzer {
	public:
		SemanticAnalyzer(ErrorHandler* errorHandler) : m_errorHandler{ errorHandler } {}
		bool analyze(ProgramFile* program);
		bool saveDefinitions(ProgramFile* program);
	private:
		// -- UTILITY --
		bool findNameGlobal(const Token& nameToken);

		// Return the index of the symbol in the local stack if it exists, if it does not then return -1
		int32_t findNameLocal(const Token& nameToken);

		bool compareArgsToParams(const std::vector<TypePtr>& paramList, std::shared_ptr<Call> call);

		void pushScope();
		void popScope();
		SymbolTable& topScope();
		std::string createUnique(const std::string& name);

		// -- DEFINITIONS --
		
		bool analyzeDefinition(DefinitionPtr definition, bool toSave = false);
		bool analyzeDefinitionFunction(DefinitionPtr definition, bool toSave = false);
		bool analyzeParameters(const ParameterList& paramList);
		bool analyzeDefinitionVariable(DefinitionPtr definition, bool toSave = false);
		bool analyzeDefinitionClass(DefinitionPtr definition, bool toSave = false);

		// -- STATEMENTS --
		bool analyzeStatement(StatementPtr statement);
		bool analyzeStatementExpression(StatementPtr statement);
		bool analyzeStatementCompound(StatementPtr statement);
		bool analyzeStatementReturn(StatementPtr statement);
		bool analyzeStatementIf(StatementPtr statement);
		bool analyzeStatementWhile(StatementPtr statement);
		bool analyzeStatementLoop(StatementPtr statement);
		bool analyzeStatementBreak(StatementPtr statement);
		bool analyzeStatementContinue(StatementPtr statement);

		// Expressions
		bool analyzeExpression(ExpressionPtr expression);
		bool analyzeExpressionInteger(ExpressionPtr expression);
		bool analyzeExpressionString(ExpressionPtr expression);
		bool analyzeExpressionCharacter(ExpressionPtr expression);
		bool analyzeExpressionFloat(ExpressionPtr expression);
		bool analyzeExpressionArray(ExpressionPtr expression);
		bool analyzeExpressionBinary(ExpressionPtr expression);
		bool analyzeExpressionUnary(ExpressionPtr expression);
		bool analyzeExpressionIdentifier(ExpressionPtr expression);
		bool analyzeExpressionCall(ExpressionPtr expression);
		bool analyzeExpressionAssignment(ExpressionPtr expression);
		bool analyzeExpressionMemberAccess(ExpressionPtr expression);
	private:
		ProgramFile* m_program{ nullptr };
		SymbolTable m_globalTable{};
		std::shared_ptr<FunctionType> m_currentFunction{ nullptr };
		std::vector<SymbolTable> m_localStack{};

		std::vector<uint8_t> m_loopStack{};

		// This will be expanded later
		std::vector<std::string> m_userDefinedTypes{};

		ErrorHandler* m_errorHandler{ nullptr };
	};
}