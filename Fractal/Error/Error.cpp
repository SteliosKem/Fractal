#include "Error.h"
#include <iostream>
#include "Utilities.h"
#include <stdlib.h>

namespace Fractal {
	uint32_t trimLeadingWhitespace(std::string& string) {
		uint32_t i = 0;
		while (i < string.size()) {
			if (string[i] == '\t' || string[i] == ' ')
				i++;
			else break;
		}
		string = string.substr(i);
		return i;
	}

	std::string color(Color color) {
		switch (color)
		{
		case Fractal::Color::Red:
			return "\033[91m";
		case Fractal::Color::White:
			return "\033[97m";
		case Fractal::Color::Purple:
			return "\033[95m";
		case Fractal::Color::LightBlue:
			return "\033[96m";
		case Fractal::Color::Bold:
			return "\033[1m";
		case Fractal::Color::Underlined:
			return "\033[4m";
		case Fractal::Color::NotUnderlined:
			return "\033[24m";
		case Fractal::Color::Default:
			return "\033[0m";
		default:
			break;
		}
	}

	void ErrorHandler::reportError(const Error& error) {
		m_errorList.push_back(error);
	}

	void ErrorHandler::outputErrors() const {
		for (const auto& error : m_errorList)
			printError(error);
	}

	bool ErrorHandler::hasErrors() const {
		return !m_errorList.empty();
	}

	void ErrorHandler::printError(const Error& error) const {

		std::cout << color(Color::Red) << color(Color::Underlined) << "Error" << color(Color::NotUnderlined) << ": " << color(Color::Default);
		std::cout << color(Color::White) << error.message << color(Color::Default) << '\n';

		// Starting and ending indices of the error in the 'line' string
		uint32_t startIndex = error.position.startIndex - error.position.lineIndexOffset;
		uint32_t endIndex = error.position.endIndex - error.position.lineIndexOffset;
		std::string filename = error.position.sourceFilePath.filename().string();

		// Name of the file, line:position of the error that occured
		std::string paddingString = filename + " " + std::to_string(error.position.line) + ":" + std::to_string(startIndex) + ":  ";

		std::cout << paddingString;

		// Get the line in which the error occured from the file
		std::string line = readLine(error.position.sourceFilePath, error.position.line);

		// Will use this to compute the new starting index, after leading whitespace is removed
		uint32_t startingIndexOffset = trimLeadingWhitespace(line);
		std::cout << line.substr(0, startIndex - startingIndexOffset) << color(Color::Red) 
			<< line.substr(startIndex - startingIndexOffset, endIndex - startingIndexOffset) 
			<< color(Color::Default) << line.substr(endIndex + 1 - startingIndexOffset) << '\n';

		for (size_t i = 0; i < paddingString.size(); i++)
			std::cout << " ";

		for (size_t i = 0; i < startIndex - startingIndexOffset; i++) {
			if (line[i] == '\t')
				std::cout << '\t';
			else
				std::cout << " ";
		}
		std::cout << color(Color::Red) << "^";
		for (size_t i = 0; i < endIndex - startIndex; i++)
			std::cout << "~";
		std::cout << color(Color::Default) << "\n";
	}
}