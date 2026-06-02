// SemanticAnalyzer.h
// Visitor-based semantic analyzer. Implements ExpressionVisitor and
// StatementVisitor so the compiler enforces exhaustive node coverage and so
// every dispatch site is statically typed (no static_pointer_cast chains).
//
// Pattern: visit() methods compute per-node state and store the result on
// instance members (m_ok, m_currentFunction, ...). Helpers analyze() and
// analyzeStmt() / analyzeDef() dispatch through accept() and short-circuit
// once m_ok flips to false.
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once

#include "Error/Error.h"
#include "Parser/Nodes.h"
#include "Parser/Visitor.h"

namespace Fractal {
    struct SymbolEntry {
        std::string name;
        TypePtr type;
    };
    using SymbolTable = std::unordered_map<std::string, SymbolEntry>;
    using GlobalNames = std::vector<std::string>;

    class SemanticAnalyzer : public ExpressionVisitor, public StatementVisitor {
    public:
        SemanticAnalyzer(ErrorHandler* errorHandler) : m_errorHandler{ errorHandler } {}

        bool analyze(ProgramFile* program);

        // ExpressionVisitor
        void visit(IntegerLiteral& node) override;
        void visit(FloatLiteral& node) override;
        void visit(StringLiteral& node) override;
        void visit(CharacterLiteral& node) override;
        void visit(ArrayList& node) override;
        void visit(BinaryOperation& node) override;
        void visit(UnaryOperation& node) override;
        void visit(Identifier& node) override;
        void visit(Call& node) override;
        void visit(Assignment& node) override;
        void visit(MemberAccess& node) override;
        void visit(CastExpression& node) override;
        void visit(DereferenceExpression& node) override;
        void visit(AddressOfExpression& node) override;

        // StatementVisitor — plain statements
        void visit(NullStatement& node) override;
        void visit(CompoundStatement& node) override;
        void visit(IfStatement& node) override;
        void visit(LoopStatement& node) override;
        void visit(WhileStatement& node) override;
        void visit(BreakStatement& node) override;
        void visit(ContinueStatement& node) override;
        void visit(ExpressionStatement& node) override;
        void visit(ReturnStatement& node) override;

        // StatementVisitor — definitions reachable via Statement dispatch
        void visit(FunctionDefinition& node) override;
        void visit(VariableDefinition& node) override;
        void visit(ClassDefinition& node) override;
        void visit(DecoratedDefinition& node) override;

    private:
        // Dispatch helpers. analyze() runs the child's accept() and short-
        // circuits if a previous step already failed.
        void analyze(ExpressionPtr& e);
        void analyze(StatementPtr& s);
        void analyzeDef(DefinitionPtr& d);

        // Scope / symbol-table helpers (unchanged from the enum-dispatch version).
        bool findNameGlobal(const Token& nameToken);
        int32_t findNameLocal(const Token& nameToken);
        bool compareArgsToParams(const std::vector<TypePtr>& paramList,
                                  std::shared_ptr<Call> call);

        void pushScope();
        void popScope();
        SymbolTable& topScope();
        std::string createUnique(const std::string& name);

        // Returns false if `original` cannot be coerced to `target`.
        bool handleTypeConversion(ExpressionPtr* a, ExpressionPtr* b);
        static bool tryCast(ExpressionPtr* original, TypePtr target);
        static void cast(ExpressionPtr* original, TypePtr target);

        // Parameters share a helper because they're not AST nodes (they live
        // off FunctionDefinition::parameterList directly).
        bool analyzeParameters(const ParameterList& paramList);

    private:
        ProgramFile* m_program{ nullptr };
        SymbolTable m_globalTable{};
        std::shared_ptr<FunctionType> m_currentFunction{ nullptr };
        std::vector<SymbolTable> m_localStack{};

        std::vector<uint8_t> m_loopStack{};
        std::vector<std::string> m_userDefinedTypes{};

        // Per-analysis monotonic counters. Reset in analyze().
        uint64_t m_uniqueIndex{ 0 };
        uint8_t m_loopIndex{ 0 };

        // Cumulative success flag. visit() methods flip this to false on
        // error and check it on entry to short-circuit further work.
        bool m_ok{ true };

        // If the current top-level Definition dispatch is in "save only"
        // mode (used by decorated definitions). Mirrors the previous toSave
        // parameter without threading it through every visit().
        bool m_saveMode{ false };

        ErrorHandler* m_errorHandler{ nullptr };
    };
}
