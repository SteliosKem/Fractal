// Type.h
// Contains certain Type related data types and functionality
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once
#include <variant>
#include <memory>

namespace Fractal {
	enum class BasicType {
		None,
		Null,
		I32,
		I64,
		F32,
		F64,
		User,
		String,
		Character,
	};

	enum class TypeInfo {
		Fundamental,
		UserDefined,
		Pointer,
		Array,
		Function,
		Empty,
	};

	struct FunctionType;

	inline std::string getBasicType(BasicType type) {
		switch (type)
		{
		case BasicType::Null: return "Null";
		case BasicType::I32: return "i32";
		case BasicType::I64: return "i64";
		case BasicType::F32: return "f32";
		case BasicType::F64: return "f64";
		case BasicType::String: return "String";
		case BasicType::Character: return "Char";
		default:
			return "";
		}
	}

	struct Type {
		virtual ~Type() = default;
		virtual TypeInfo typeInfo() const { return TypeInfo::Empty; }
		virtual std::string typeName() const { return ""; }
	};

	using TypePtr = std::shared_ptr<Type>;

	class FundamentalType : public Type {
	public:
		FundamentalType(BasicType type) : type{ type } {}

		TypeInfo typeInfo() const override { return TypeInfo::Fundamental; }
		std::string typeName() const override {
			return getBasicType(type);
		}
	public:
		BasicType type;
	};

	class UserDefinedType : public Type {
	public:
		UserDefinedType(std::string name) : name{ name } {}
		
		TypeInfo typeInfo() const override { return TypeInfo::UserDefined; }
		std::string typeName() const override {
			return name;
		}
	public:
		std::string name;
	};

	class FunctionType : public Type {
	public:
		FunctionType(TypePtr returnType, std::vector<TypePtr>& parameterTypes) : returnType{ returnType }, parameterTypes{ parameterTypes } {}

		TypeInfo typeInfo() const override { return TypeInfo::Function; }
		std::string typeName() const override {
			std::string toReturn = "func<" + returnType->typeName() + "(";
			for (auto type : parameterTypes)
				toReturn += type->typeName() + ", ";
			toReturn += ")";
			return toReturn;
		}
	public:
		TypePtr returnType;
		std::vector<TypePtr> parameterTypes;
	};

	class PointerType : public Type {
	public:
		PointerType(TypePtr pointingType) : pointingType{ pointingType } {}

		TypeInfo typeInfo() const override { return TypeInfo::Pointer; }
		std::string typeName() const override {
			return "(" + pointingType->typeName() + ")";
		}
	public:
		TypePtr pointingType;
	};

	class ArrayType : public Type {
	public:
		ArrayType(TypePtr arrayType) : arrayType{ arrayType } {}
		
		TypeInfo typeInfo() const override { return TypeInfo::Array; }
		std::string typeName() const override {
			return "[" + arrayType->typeName() + "]";
		}
	public:
		TypePtr arrayType;
	};

	class EmptyType : public Type {
		TypeInfo typeInfo() const override { return TypeInfo::Empty; }
	};

	inline bool sameType(const TypePtr a, const TypePtr b) {
		if (a->typeInfo() != b->typeInfo()) return false;

		switch (a->typeInfo()) {
			case TypeInfo::Fundamental: return static_pointer_cast<FundamentalType>(a)->type == static_pointer_cast<FundamentalType>(b)->type;
			case TypeInfo::UserDefined: return static_pointer_cast<UserDefinedType>(a)->name == static_pointer_cast<UserDefinedType>(b)->name;
			case TypeInfo::Array: return sameType(static_pointer_cast<ArrayType>(a)->arrayType, static_pointer_cast<ArrayType>(b)->arrayType);
			case TypeInfo::Pointer: return sameType(static_pointer_cast<PointerType>(a)->pointingType, static_pointer_cast<PointerType>(b)->pointingType);
			case TypeInfo::Function: {
				std::shared_ptr<FunctionType> a_ = static_pointer_cast<FunctionType>(a);
				std::shared_ptr<FunctionType> b_ = static_pointer_cast<FunctionType>(b);
				if (!sameType(a_->returnType, b_->returnType)) return false;
				if (a_->parameterTypes.size() != b_->parameterTypes.size()) return false;
				for (size_t i = 0; i < a_->parameterTypes.size(); i++)
					if (!sameType(a_->parameterTypes[i], b_->parameterTypes[i])) return false;
			}
			default: return false;
		}
		return true;
	}
}