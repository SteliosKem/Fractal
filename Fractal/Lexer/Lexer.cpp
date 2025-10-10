// Lexer.cpp
// Contains the Lexer Class method implementations
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "Utilities.h"
#include "Lexer.h"
#include <iostream>

namespace Fractal {
	Lexer::Lexer(ErrorHandler* handler) : m_errorHandler{ handler } {}

	const std::vector<Token>& Lexer::getTokenList() {
		return m_tokens;
	}

	void Lexer::print() {
		print(m_tokens);
	}

	void Lexer::print(const std::vector<Token>& tokens) {
		// Pretty prints in this format: "<Line>     | <TokenType>     | <TokenValue>"
		std::cout << "Line:\t  TokenType:\t  Value:\n";

		// First line is set to an impossible number in order to be updated immediately at the start of the loop
		int currentLine = -1;
		for (auto& token : tokens) {
			// Output 'Repeating' Symbol '|' if the line did not change since the previous token
			if (token.position.line != currentLine) {
				currentLine = token.position.line;
				std::cout << std::to_string(currentLine);
			}
			else
				std::cout << "|";
			std::cout << "\t| " << token.type << "\t\t| " << token.value << "\n";
		}
	}

	void Lexer::advance(uint32_t times) {
		// If new index does not exceed the length of the source file then set the current character
		// to the one of the source string at the new index, else set it to a null character, indicating End of File
		for (uint32_t i = 0; i < times; i++) {
			m_currentSourceIndex++;
			if (m_currentSourceIndex < m_sourceCode.size())
				m_currentCharacter = m_sourceCode[m_currentSourceIndex];
			else
				m_currentCharacter = '\0';
			m_currentPosition.startIndex = m_currentSourceIndex;
			m_currentPosition.endIndex = m_currentSourceIndex;
		}
	}

	char Lexer::peek(uint32_t depth) const {
		// If current index + depth does not exceed the length of the source file then return the character
		// of the source string at the desired index, else set return a null character
		if (m_currentSourceIndex + depth < m_sourceCode.size())
			return m_sourceCode[m_currentSourceIndex + depth];
		else
			return '\0';
	}

	bool Lexer::match(char character) {
		if (peek() == character) {
			advance();
			return true;
		}
		return false;
	}

	char Lexer::currentCharacter() const {
		// Accessing current character through a function in case other functionality is needed here
		return m_currentCharacter;
	}

	void Lexer::handleNewline() {
		m_currentLine++;
		m_currentPosition.line = m_currentLine;
		m_currentPosition.lineIndexOffset = m_currentSourceIndex + 1;
		advance();
	}

	void Lexer::handleWhitespace() {
		// While current character is whitespace, skip it
		while (true) {
			switch (currentCharacter())
			{
				// If character is space or tab just advance
			case ' ':
			case '\t':
				advance();
				break;
				// In case of a newline, increase the current line index
			case '\n':
				handleNewline();
				break;
			case '/':
				// Handle Single-Line comments
				if (peek() == '/') {
					while (currentCharacter() != '\n' || currentCharacter() == '\0')
						advance();
					if (currentCharacter() == '\0')
						return;
					handleNewline();
					continue;
				}
				// Handle Multi-Line comments
				else if (peek() == '*') {
					while (true) {
						if (currentCharacter() == '\0')
							return;
						if (currentCharacter() == '*' && peek() == '/') {
							advance(2);
							break;
						}
						if(currentCharacter() == '\n')
							handleNewline();
						else
							advance();
					}
					continue;
				}
				// If the slash turned out to not be a comment, stop checking for whitespace
				else
					return;
			default:
				return;
			}
		}
	}

	Token Lexer::lex() {
		// Skip over any whitespace
		handleWhitespace();

		if (isDigit(currentCharacter())) return makeNumberToken();
		if (isLetter(currentCharacter())) return makeNameToken();

		Position position = m_currentPosition;

		// Position that has the end index increased, in case the Token will be of double characters
		Position nextPosition = position;
		nextPosition.endIndex++;

		Token to_return;

#define SINGLE_CHARACTER(x, y) case x: advance(); return Token{ y, std::string() + x, position }
#define DOUBLE_OR_SINGLE(character, first_char_name, second_char_match, double_char_str, double_char_name) case character: to_return = match(second_char_match) ? Token{ double_char_name, double_char_str, nextPosition } : Token{ first_char_name, std::string() + character, position }; advance(); return to_return;

		switch (currentCharacter())
		{
			SINGLE_CHARACTER('(', LEFT_PARENTHESIS);
			SINGLE_CHARACTER(')', RIGHT_PARENTHESIS);
			SINGLE_CHARACTER('{', LEFT_BRACE);
			SINGLE_CHARACTER('}', RIGHT_BRACE);
			SINGLE_CHARACTER('[', LEFT_BRACKET);
			SINGLE_CHARACTER(']', RIGHT_BRACKET);
			SINGLE_CHARACTER(';', SEMICOLON);
			SINGLE_CHARACTER(',', COMMA);
			SINGLE_CHARACTER('.', DOT);
			SINGLE_CHARACTER('^', CAP);
			SINGLE_CHARACTER('&', AMPERSAND);
			SINGLE_CHARACTER('~', TILDE);
			SINGLE_CHARACTER('%', PERCENT);
			SINGLE_CHARACTER(':', COLON);
			// Check for double character tokens
			DOUBLE_OR_SINGLE('+', PLUS, '=', "+=", PLUS_EQUAL);
			DOUBLE_OR_SINGLE('*', STAR, '=', "*=", STAR_EQUAL);
			DOUBLE_OR_SINGLE('/', SLASH, '=', "/=", SLASH_EQUAL);
			DOUBLE_OR_SINGLE('!', BANG, '=', "!=", BANG_EQUAL);
			case '=':	/* Match Double Arrow */ if (match('>')) { advance(); return Token{ DOUBLE_ARROW, "=>", nextPosition }; }
					to_return = match('=') ? Token{ EQUAL_EQUAL, "==", nextPosition } : Token{ EQUAL, "=", position }; advance(); return to_return;
			DOUBLE_OR_SINGLE('<', LESS, '=', "<=", LESS_EQUAL);
			DOUBLE_OR_SINGLE('>', GREATER, '=', ">=", GREATER_EQUAL);
			case '-':	/* Match Arrow */ if (match('>')) { advance(); return Token{ ARROW, "->", nextPosition }; }
					to_return = match('=') ? Token{ MINUS_EQUAL, "-=", nextPosition } : Token{ MINUS, "-", position }; advance(); return to_return;
			case '\'':
			case '"':	
				return makeStringToken(currentCharacter());
			case '\0':	return Token{ SPECIAL_EOF, "EOF", position };
			default:
				break;
		}

		// If this part of the code is reached, then the current character does not match any known one, so an adequate error is reported.

		m_errorHandler->reportError({ "Unkown Character '" + std::string(1, currentCharacter()) + "'", position });
		return Token{ SPECIAL_ERROR, "", position };
	}

	Token Lexer::makeNumberToken() {
		std::string numberValue{ "" };
		bool isFloatingPoint{ false };

		// Store position for start index
		Position position = m_currentPosition;

		// Make a number from all digits next to eachother in the string
		while (isDigit(currentCharacter()) || currentCharacter() == '.') {
			if (currentCharacter() == '.') {
				// If there was already a dot in this number, report an unexpected dot error
				if (isFloatingPoint) {
					m_errorHandler->reportError(Error{ "Unexpected '.'", m_currentPosition });
					return Token{ SPECIAL_ERROR, "Unexpected '.'", m_currentPosition };
				}
				else {
					isFloatingPoint = true;
					numberValue += currentCharacter();
					advance();
				}
			}
			else {
				numberValue += currentCharacter();
				advance();
			}
		}

		position.endIndex = m_currentSourceIndex - 1;

		return isFloatingPoint ? Token{ TYPE_FLOAT, numberValue, position } : Token{ TYPE_INTEGER, numberValue, position };
	}

	Token Lexer::makeNameToken() {
		std::string nameString{ "" };

		// Store position for start index
		Position position = m_currentPosition;

		// Make name from all characters next to eachother in the string
		while (isAlphanumeric(currentCharacter())) {
			nameString += currentCharacter();
			advance();
		}

		position.endIndex = m_currentSourceIndex;

		TokenType keywordType = getKeyword(nameString);
		return keywordType == NO_TYPE ? Token{ IDENTIFIER, nameString, position } : Token{ keywordType, nameString, position };
	}

	Token Lexer::makeStringToken(char character) {
		std::string string{ "" };
		// Store position for start index
		Position position = m_currentPosition;

		// For error reporting if there is an unterminated string
		Position lastPos = m_currentPosition;
		advance();
		// Make name from all characters next to eachother in the string
		while (currentCharacter() != character && currentCharacter() != '\0' && currentCharacter() != '\n') {
			lastPos = m_currentPosition;
			string += currentCharacter();
			advance();
		}

		if (currentCharacter() != character) m_errorHandler->reportError({ "Unterminated string or character literal", lastPos });

		position.endIndex = m_currentSourceIndex;
		advance();
		return character == '"' ? Token{STRING_LITERAL, string, position} : Token{CHARACTER_LITERAL, string, position};
	}

	bool Lexer::analyze(const std::filesystem::path& filepath) {
		// Reset internal values
		m_currentSourceIndex = -1;
		if (!std::filesystem::exists(filepath)) {
			m_errorHandler->reportError(Error("No valid file specified.", {}));
			return false;
		}
		m_sourceCode = readFile(filepath);
		m_currentLine = 1;
		m_currentPosition.line = 1;
		m_currentPosition.sourceFilePath = filepath;
		m_tokens.clear();

		advance();

		// Lex until End of File
		while (currentCharacter() != '\0' && !m_errorHandler->hasErrors())
			m_tokens.push_back(lex());

		return !m_errorHandler->hasErrors();
	}
}