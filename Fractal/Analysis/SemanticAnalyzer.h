// SemanticAnalyzer.h
// Contains the SemanticAnalyzer Class definition
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once

#include "Error/Error.h"
#include "Parser/Nodes.h"

namespace Fractal {
	using SymbolTable = std::unordered_map<std::string, Type>;
	using GlobalNames = std::vector<std::string>;

	class SemanticAnalyzer {
	public:
		SemanticAnalyzer(ErrorHandler* errorHandler) : m_errorHandler{ errorHandler } {}
		bool analyze(ProgramFile* program);
	private:
		// -- DEFINITIONS --
		
		bool analyzeDefinition(DefinitionPtr definition);
		bool analyzeDefinitionFunction(DefinitionPtr definition);
		bool analyzeParameters(const ParameterList& paramList);
		bool analyzeDefinitionVariable(DefinitionPtr definition);
		bool analyzeDefinitionClass(DefinitionPtr definition);

		// -- STATEMENTS --
		bool analyzeStatement(StatementPtr statement);
		bool analyzeStatementExpression(StatementPtr statement);

		// Expressions
		bool analyzeExpression(ExpressionPtr expression);
	private:
		ProgramFile* m_program{ nullptr };
		GlobalNames m_globals;
		ErrorHandler* m_errorHandler{ nullptr };
	};
}