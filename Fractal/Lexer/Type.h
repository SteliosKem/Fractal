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
	};

	enum class TypeInfo {
		Fundamental,
		UserDefined,
		Pointer,
		Array
	};

	struct Type {
		BasicType basicType;
		TypeInfo typeInfo;
	};
}