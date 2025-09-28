// Lexer.h
// Contains the Lexer Class definition
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include "Token.h"
#include "Common.h"

namespace Fractal {
	class Lexer {
	public:
		Lexer(ErrorHandler* handler);

		// Tokenize whole source code, return if there were any errors
		bool analyze(const std::filesystem::path& file_path);

		// Print the list of tokens to the console in a readable way
		void print();
		static void print(const std::vector<Token>& tokens);

		const std::vector<Token>& getTokenList();
	private:
		// -- Utility --

		// Move to the next character in the source file
		void advance(uint32_t times = 1);

		// Returns the character at (current + <depth>) index
		char peek(uint32_t depth = 1) const;

		// Return true and advance if next character matches the given one
		bool match(char character);

		// Returns the character of the source string at the current index
		char currentCharacter() const;


		// -- Lexical Analysis --


		// Skips over any whitespace characters
		void handleWhitespace();
		void handleNewline();

		// Returns a token depending on the current character(s)
		Token lex();

		// Create a floating point or integer literal token
		Token makeNumberToken();

		// Create a string literal token
		Token makeStringToken(char character);

		// Create a keyword or identifier token
		Token makeNameToken();

	private:
		int32_t m_currentSourceIndex{ -1 };
		char m_currentCharacter{ '\0' };
		uint32_t m_currentLine{ 0 };

		Position m_currentPosition{};

		std::filesystem::path m_filePath{ "" };
		std::string m_sourceCode{ "" };
		std::vector<Token> m_tokens{};
		ErrorHandler* m_errorHandler{ nullptr };
	};
}