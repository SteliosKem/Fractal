// Visitor.h
// Abstract visitor interfaces for the AST. Two flavours:
//
//   ExpressionVisitor — one visit() per concrete Expression subclass.
//   StatementVisitor  — one visit() per concrete Statement subclass.
//                       (Definitions are also Statements, so their visit()
//                        methods live on StatementVisitor too.)
//
// Each interface method is pure virtual: a consumer that fails to handle a
// new node fails to compile, which is the whole point — the compiler
// enforces exhaustiveness rather than `default: return false;` swallowing it.
//
// Visit methods are non-const so consumers can mutate AST nodes (e.g. attach
// inferred types, replace expressions with Cast wrappers).
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once

namespace Fractal {

// -- Forward declarations of every concrete AST class --------------------
// Listing them here means Visitor.h compiles without including Nodes.h,
// which in turn makes the include graph DAG-shaped: Nodes.h -> Visitor.h,
// not the other way around.

// Expressions
class IntegerLiteral;
class FloatLiteral;
class StringLiteral;
class CharacterLiteral;
class ArrayList;
class BinaryOperation;
class UnaryOperation;
class Identifier;
class Call;
class Assignment;
class MemberAccess;
class CastExpression;
class DereferenceExpression;
class AddressOfExpression;

// Statements
class NullStatement;
class CompoundStatement;
class IfStatement;
class LoopStatement;
class WhileStatement;
class BreakStatement;
class ContinueStatement;
class ExpressionStatement;
class ReturnStatement;

// Definitions (also valid as statements when they appear in a local scope)
class FunctionDefinition;
class VariableDefinition;
class ClassDefinition;
class DecoratedDefinition;

class ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() = default;

    virtual void visit(IntegerLiteral &) = 0;
    virtual void visit(FloatLiteral &) = 0;
    virtual void visit(StringLiteral &) = 0;
    virtual void visit(CharacterLiteral &) = 0;
    virtual void visit(ArrayList &) = 0;
    virtual void visit(BinaryOperation &) = 0;
    virtual void visit(UnaryOperation &) = 0;
    virtual void visit(Identifier &) = 0;
    virtual void visit(Call &) = 0;
    virtual void visit(Assignment &) = 0;
    virtual void visit(MemberAccess &) = 0;
    virtual void visit(CastExpression &) = 0;
    virtual void visit(DereferenceExpression &) = 0;
    virtual void visit(AddressOfExpression &) = 0;
};

class StatementVisitor {
public:
    virtual ~StatementVisitor() = default;

    // Plain statements
    virtual void visit(NullStatement &) = 0;
    virtual void visit(CompoundStatement &) = 0;
    virtual void visit(IfStatement &) = 0;
    virtual void visit(LoopStatement &) = 0;
    virtual void visit(WhileStatement &) = 0;
    virtual void visit(BreakStatement &) = 0;
    virtual void visit(ContinueStatement &) = 0;
    virtual void visit(ExpressionStatement &) = 0;
    virtual void visit(ReturnStatement &) = 0;

    // Definitions (also reachable as local statements)
    virtual void visit(FunctionDefinition &) = 0;
    virtual void visit(VariableDefinition &) = 0;
    virtual void visit(ClassDefinition &) = 0;
    virtual void visit(DecoratedDefinition &) = 0;
};

} // namespace Fractal
