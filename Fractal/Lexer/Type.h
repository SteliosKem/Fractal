// Type.h
// Contains certain Type related data types and functionality
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once

namespace Fractal {
	enum class BasicType {
		Null,
		I32,
		I64,
		F32,
		F64,
		User,
	};

	enum class TypeInfo {
		Fundamental,
		UserDefined,
		Pointer,
		Array,
		Function
	};

	struct Type {
		BasicType basicType;
		TypeInfo typeInfo;

		// Only used in user defined types and functions
		std::string name{ "" };
	};
}