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


	class ErrorHandler {
	public:
		ErrorHandler() = default;

		void reportError(const Error& error);
		void outputErrors() const;
		bool hasErrors() const;
		void clearErrors();
	private:
		void printError(const Error& error) const;

	private:
		std::vector<Error> m_errorList{};
	};
}