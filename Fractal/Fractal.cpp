#include "Fractal.h"

int main()
{
	Fractal::ErrorHandler errorHandler;
	Fractal::Lexer lexer(&errorHandler);
	if (!lexer.analyze("../../../../Testing/Source.frc")) {
		errorHandler.outputErrors();
		return EXIT_FAILURE;
	}

	lexer.print();
	return EXIT_SUCCESS;
}
