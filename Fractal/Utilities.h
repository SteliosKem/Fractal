#pragma once
#include "Common.h"

namespace Fractal {
	// Returns contents of a file at the specified path
	std::string readFile(const std::filesystem::path& path);

	// Read only a certain line in a file, for error display purposes
	std::string readLine(const std::filesystem::path& path, uint32_t lineIndex);

	// Write contents of string to file at specified path
	void writeFile(const std::string& source, const std::filesystem::path& path);

	// Check if character is a digit
	bool isDigit(char character);
	// Check if character is a letter or underscore
	bool isLetter(char character);
	// Check if character is a letter or underscore or digit
	bool isAlphanumeric(char character);
}