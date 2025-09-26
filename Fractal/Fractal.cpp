#include "Fractal.h"

int main()
{
	Fractal::ErrorHandler errorHandler;
	Fractal::Lexer lexer(&errorHandler);
	Fractal::Parser parser(&errorHandler);
	if (!lexer.analyze("../../../../Testing/Source.frc")) {
		errorHandler.outputErrors();
		return EXIT_FAILURE;
	}
	lexer.print();

	if(!parser.parse(lexer.getTokenList())) {
		errorHandler.outputErrors();
		return EXIT_FAILURE;
	}
	parser.statements()[0]->print();
	return EXIT_SUCCESS;
}
