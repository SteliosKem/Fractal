#include "Utilities.h"
#include <fstream>
#include <iostream>

namespace Fractal {
	std::string readFile(const std::filesystem::path& path) {
		std::ifstream file{ path };
		auto fileSize = file.seekg(0, std::ios::end).tellg();
		file.seekg(0);

		std::string fileContents{};
		fileContents.reserve(fileSize);

		std::string line;
		while (std::getline(file, line))
			fileContents += line + '\n';

		return fileContents;
	}

	std::string readLine(const std::filesystem::path& path, uint32_t lineIndex) {
		std::ifstream file{ path };
		auto fileSize = file.seekg(0, std::ios::end).tellg();
		file.seekg(0);

		std::string fileContents{};
		fileContents.reserve(fileSize);

		uint32_t currentLine = 0;

		std::string line;
		while (!file.eof()) {
			currentLine++;
			std::getline(file, line);
			if (currentLine == lineIndex) break;
		}

		file.close();

		return line;
	}

	void writeFile(const std::string& source, const std::filesystem::path& path) {
		std::ofstream file{ path };
		file << source;
		file.close();
	}

	bool isDigit(char character) {
		return character >= '0' && character <= '9';
	}

	bool isLetter(char character) {
		return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_';
	}

	bool isAlphanumeric(char character) {
		return isDigit(character) || isLetter(character);
	}
}