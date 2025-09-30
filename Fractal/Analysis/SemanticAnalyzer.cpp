// SemanticAnalyzer.h
// Contains the SemanticAnalyzer Class member definitions
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "SemanticAnalyzer.h"

namespace Fractal {
	bool SemanticAnalyzer::analyze(ProgramFile* program) {
		m_program = program;

		for (auto& definition : m_program->definitions)
			if (!analyzeDefinition(std::move(definition))) return false;

		for (auto& statement : m_program->statements)
			if (!analyzeStatement(std::move(statement))) return false;

		return true;
	}

	bool SemanticAnalyzer::analyzeDefinition(DefinitionPtr definition) {
		switch (definition->getType()) {
			case NodeType::FunctionDefinition: return analyzeDefinitionFunction(std::move(definition));
			case NodeType::VariableDefinition: return analyzeDefinitionVariable(std::move(definition));
			case NodeType::ClassDefinition: return analyzeDefinitionClass(std::move(definition));
		}
	}

	bool SemanticAnalyzer::analyzeDefinitionFunction(DefinitionPtr definition) {
		FunctionDefinition* functionDefinition = static_cast<FunctionDefinition*>(definition.get());

		if (std::find(m_globals.begin(), m_globals.end(), functionDefinition->nameToken.value) != m_globals.end()) {
			m_errorHandler->reportError({ "Function '" + functionDefinition->nameToken.value + "' is already defined", functionDefinition->nameToken.position });
			return false;
		}

		m_globals.push_back(functionDefinition->nameToken.value);

		if (!analyzeParameters(functionDefinition->parameterList)) return false;
	}

	bool SemanticAnalyzer::analyzeParameters(const ParameterList& paramList) {
		std::vector<std::string> paramListCheck;
		for (const auto& parameter : paramList) {
			if (std::find(m_globals.begin(), m_globals.end(), parameter->nameToken.value) != m_globals.end())
				m_errorHandler->reportWarning({ "Parameter '" + parameter->nameToken.value + "' shadows a global name", parameter->nameToken.position });
			if (std::find(paramListCheck.begin(), paramListCheck.end(), parameter->nameToken.value) != paramListCheck.end()) {
				m_errorHandler->reportError({ "Parameter '" + parameter->nameToken.value + "' is already defined", parameter->nameToken.position });
				return false;
			}
			paramListCheck.push_back(parameter->nameToken.value);
		}
		return true;
	}
	bool SemanticAnalyzer::analyzeDefinitionVariable(DefinitionPtr definition) { return true; }
	bool SemanticAnalyzer::analyzeDefinitionClass(DefinitionPtr definition) { return true; }

	// -- STATEMENTS --
	bool SemanticAnalyzer::analyzeStatement(StatementPtr statement) { return true; }
	bool SemanticAnalyzer::analyzeStatementExpression(StatementPtr statement) { return true; }

	// Expressions
	bool SemanticAnalyzer::analyzeExpression(ExpressionPtr expression) { return true; }
}