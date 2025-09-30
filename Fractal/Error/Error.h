// Error.h
// Contains the Error and ErrorHandler Class definitions which are used throughout the project for reporting and outputting errors
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include "Common.h"
#include "Position.h"

namespace Fractal {
	enum class Color {
		Red,
		White,
		Purple,
		LightBlue,
		Bold,
		Underlined,
		NotUnderlined,
		Default
	};

	class Error {
	public:
		Error(const std::string& message, const Position& position) : message{ message }, position{ position } {}

	public:
		std::string message;
		Position position;
	};

	enum class ErrorType {
		Error,
		Warning
	};

	class ErrorHandler {
	public:
		ErrorHandler() = default;

		void reportError(const Error& error);
		void reportWarning(const Error& error);
		void outputErrors() const;
		void outputWarnings() const;
		bool hasErrors() const;
		void clearErrors();
	private:
		void print(const Error& error, ErrorType type) const;
	private:
		std::vector<Error> m_errorList{};
		std::vector<Error> m_warningList{};
	};
}