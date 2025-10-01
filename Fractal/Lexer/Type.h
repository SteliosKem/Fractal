// Type.h
// Contains certain Type related data types and functionality
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include <variant>
#include <memory>

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

	struct FunctionType;

	struct Type {
		std::variant<BasicType, std::shared_ptr<Type>, std::shared_ptr<FunctionType>> innerType;
		TypeInfo typeInfo;

		// Only used in user defined types and functions
		std::string name{ "" };
	};

	using TypePtr = std::shared_ptr<Type>;

	struct FunctionType {
		TypePtr returnType;
		std::vector<TypePtr> parameterTypes;
	};
}