// Token.h
// Contains Token related type definitions and functionality
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include "Common.h"
#include "Error/Position.h"
#include "Type.h"

namespace Fractal {
	enum TokenType {
		// -- Single Character Tokens --
			// Grouped Tokens
			LEFT_PARENTHESIS,
			RIGHT_PARENTHESIS,

			LEFT_BRACE,
			RIGHT_BRACE,

			LEFT_BRACKET,
			RIGHT_BRACKET,

			// Arithmetic
			PLUS,
			MINUS,
			STAR,
			SLASH,
			CAP,
			PERCENT,

			// Logic
			AMPERSAND,
			TILDE,
			PIPE,
			BANG,

			// Comparison
			GREATER,
			LESS,

			// Miscellaneous
			DOT,
			COMMA,
			SEMICOLON,
			COLON,

			// Assignment
			EQUAL,
		// --

		// -- Double Character Tokens --
			// Comparison
			BANG_EQUAL,
			EQUAL_EQUAL,
			GREATER_EQUAL,
			LESS_EQUAL,

			// Compound Assignment
			PLUS_EQUAL,
			MINUS_EQUAL,
			STAR_EQUAL,
			SLASH_EQUAL,

			ARROW,
			DOUBLE_ARROW,
		// --

		// -- Keywords --
			LET,
			AND,
			OR,
			TRUE,
			FALSE,
			IF,
			ELSE,
			WHILE,
			LOOP,
			FOR,
			RETURN,
			FUNCTION,
			DO,
			BREAK,
			CONTINUE,
			CLASS,
			PRIVATE,
			PUBLIC,
			THIS,
			ENUM,
			EXTERNAL,
			GLOBAL,
			INTERNAL,
			MATCH,
			CONST,
			KEY_I8,
			KEY_I16,
			KEY_I32,
			KEY_I64,
			KEY_F32,
			KEY_F64,
			KEY_BOOL,
			KEY_NULL,
		//--

		// -- Types --
			IDENTIFIER,
			STRING_LITERAL,
			CHARACTER_LITERAL,
			TYPE_I8,
			TYPE_I16,
			TYPE_I32,
			TYPE_I64,
			TYPE_F32,
			TYPE_F64,
			TYPE_BOOL,
			TYPE_INTEGER,
			TYPE_FLOAT,
		// --

		// -- Special --
			SPECIAL_EOF,
			SPECIAL_ERROR,
			NO_TYPE,
	};

	struct Token {
		TokenType type{ NO_TYPE };
		std::string value{ "" };

		Position position{};
	};

	const std::unordered_map<std::string, TokenType> KEYWORD_MAP = {
		{ "let"			, LET		},
		{ "and"			, AND		},
		{ "or"			, OR		},
		{ "true"		, TRUE		},
		{ "false"		, FALSE		},
		{ "if"			, IF		},
		{ "else"		, ELSE		},
		{ "while"		, WHILE		},
		{ "loop"		, LOOP		},
		{ "for"			, FOR		},
		{ "return"		, RETURN	},
		{ "fn"			, FUNCTION	},
		{ "external"	, EXTERNAL	},
		{ "internal"	, INTERNAL	},
		{ "global"		, GLOBAL	},
		{ "do"			, DO		},
		{ "break"		, BREAK		},
		{ "continue"	, CONTINUE	},
		{ "class"		, CLASS		},
		{ "private"		, PRIVATE	},
		{ "public"		, PUBLIC	},
		{ "this"		, THIS		},
		{ "enum"		, ENUM		},
		{ "i8"			, KEY_I8	},
		{ "i16"			, KEY_I16	},
		{ "i32"			, KEY_I32	},
		{ "i64"			, KEY_I64	},
		{ "f32"			, KEY_F32	},
		{ "f64"			, KEY_F64	},
		{ "bool"		, KEY_BOOL	},
		{ "null"		, KEY_NULL	},
		{ "match"		, MATCH		},
		{ "const"		, CONST		},
	};

	// Get Keyword TokenType from string if it exists
	inline TokenType getKeyword(const std::string& name) {
		if (KEYWORD_MAP.find(name) != KEYWORD_MAP.end())
			return KEYWORD_MAP.at(name);
		return NO_TYPE;
	}

	// Check if a token is of type Type
	inline bool isTypeToken(const Token& token) {
		switch (token.type)
		{
		case KEY_I8:
		case KEY_I16:
		case KEY_I32:
		case KEY_I64:
		case KEY_F32:
		case KEY_F64:
		case KEY_BOOL:
		case KEY_NULL:
			return true;
		default:
			return false;
		}
	}

	inline BasicType getBasicType(const Token& token) {
		switch (token.type)
		{
		case KEY_I8:
		case KEY_I16:
		case KEY_I32:
			return BasicType::I32;
		case KEY_I64:
			return BasicType::I64;
		case KEY_F32:
			return BasicType::F64;
		case KEY_F64:
			return BasicType::F32;
		case KEY_BOOL:
			return BasicType::I32;
		case KEY_NULL:
		default:
			return BasicType::Null;
		}
	}

	using TokenList = std::vector<Token>;
}