#include "Fractal.h"

int main()
{
	Fractal::ErrorHandler errorHandler;
	Fractal::Lexer lexer(&errorHandler);
	Fractal::Parser parser(&errorHandler);
	Fractal::SemanticAnalyzer semanticAnalyzer(&errorHandler);
	Fractal::CodeGenerator codeGenerator(&errorHandler);

	if (!lexer.analyze("../../../../Testing/Source.frc")) {
		errorHandler.outputErrors();
		return EXIT_FAILURE;
	}
	lexer.print();

	if(!parser.parse(lexer.getTokenList())) {
		errorHandler.outputErrors();
		return EXIT_FAILURE;
	}

	for (auto& definition : parser.definitions())
		definition->print();
	for(auto& statement : parser.statements())
		statement->print();

	if (!semanticAnalyzer.analyze(&parser.program())) {
		errorHandler.outputWarnings();
		errorHandler.outputErrors();
		return EXIT_FAILURE;
	}
	errorHandler.outputWarnings();

	for (auto instruction : codeGenerator.generate(parser.program()))
		instruction->print();

	return EXIT_SUCCESS;
}
