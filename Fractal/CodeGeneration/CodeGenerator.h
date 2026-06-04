// CodeGenerator.h
// Visitor-based IR generator. Implements ExpressionVisitor and
// StatementVisitor so each node kind is handled by a statically-typed visit()
// method, with no static_pointer_cast on dispatch.
//
// Because visit() must return void, expression results are stashed on
// m_result and retrieved by the generate() helper. Instruction-emitting
// visit() methods take the current target list via m_currentList.
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#pragma once

#include "Error/Error.h"
#include "Instructions.h"
#include "Parser/Nodes.h"
#include "Parser/Visitor.h"

namespace Fractal {

enum class Platform {
    Win,
    Mac
};

class CodeGenerator : public ExpressionVisitor, public StatementVisitor {
public:
    CodeGenerator(ErrorHandler* errorHandler) : m_errorHandler{ errorHandler } {}

    const InstructionList& generate(const ProgramFile& program, Platform platform);
    const InstructionList& instructions() const { return m_instructions; }
    const std::vector<std::string>* externals() const { return &m_externals; }

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

    // StatementVisitor — definitions
    void visit(FunctionDefinition& node) override;
    void visit(VariableDefinition& node) override;
    void visit(ClassDefinition& node) override;
    void visit(DecoratedDefinition& node) override;

private:
    // Visit `node` and return the OperandPtr it produced. Saves and restores
    // m_result so nested expression evaluations compose correctly. Takes a
    // raw pointer so callers can pass `expr.get()` without moving the
    // unique_ptr; passing nullptr is allowed.
    OperandPtr generate(Expression* expression);

    // Reports an "unimplemented codegen for X" error and resets m_result. Used
    // by every visit() that hasn't been lowered yet, so unsupported language
    // features fail loudly instead of silently producing broken assembly.
    void notImplemented(const std::string& nodeName, const Position& pos);

    // Emit a single instruction into the active target list.
    void emit(InstructionPtr instr) { m_currentList->push_back(std::move(instr)); }

    // -- Helpers grouped by node kind (called from visit()) -----------------
    OperandPtr arithmetic(BinaryOperation& node);
    OperandPtr relational(BinaryOperation& node);
    OperandPtr logical(BinaryOperation& node);
    OperandPtr idiv(BinaryOperation& node);

    // -- Instruction factories ----------------------------------------------
    InstructionPtr move(OperandPtr source, OperandPtr destination);
    InstructionPtr negate(OperandPtr source);
    InstructionPtr bitwiseNot(OperandPtr source);
    InstructionPtr add(OperandPtr destination, OperandPtr other);
    InstructionPtr sub(OperandPtr destination, OperandPtr other);
    InstructionPtr mul(OperandPtr destination, OperandPtr other);
    InstructionPtr cmp(OperandPtr left, OperandPtr right);
    InstructionPtr set(OperandPtr operand, ComparisonType type);
    InstructionPtr jmp(const std::string& label, ComparisonType type);
    InstructionPtr call(const std::string& func);
    InstructionPtr push(OperandPtr src);
    OperandPtr reg(Register register_, Size size = Size::DWord);
    OperandPtr intConst(int64_t integer);
    InstructionPtr label(const std::string& name);

    // -- Stack --------------------------------------------------------------
    int64_t allocateStack(Size size);

    // -- Validation -- second pass over emitted instructions ---------------
    void validateInstructions(InstructionList* instructions);
    void validateFunction(Instruction* instruction);
    void validateMove(InstructionList* instructions, size_t i);
    void validateAdd(InstructionList* instructions, size_t i);
    void validateSub(InstructionList* instructions, size_t i);
    void validateMul(InstructionList* instructions, size_t i);
    void validateDiv(InstructionList* instructions, size_t i);
    void validateCmp(InstructionList* instructions, size_t i);
    void validatePush(InstructionList* instructions, size_t i);
    void validateMoveOperands(InstructionList* instructions, size_t i,
                              OperandPtr source, OperandPtr* destination);
    void validateBinOperands(InstructionList* instructions, size_t i,
                             OperandPtr source, OperandPtr* other);

    // -- Label counters -----------------------------------------------------
    uint64_t generateComparisonIndex();
    uint64_t generateIfIndex();
    uint64_t generateLoopIndex();

private:
    struct LoopInfo {
        std::string startLabel;
        std::string exitLabel;
    };

    InstructionList m_instructions{};
    ProgramFile* m_program{ nullptr };  // non-owning, borrowed from caller
    int64_t m_currentStackIndex{};
    std::unordered_map<std::string, OperandPtr> m_localVarMap{};
    uint64_t m_currentComparisonIndex{};
    uint64_t m_currentIfIndex{};
    uint64_t m_currentLoopIndex{};
    std::vector<LoopInfo> m_loopStack{};
    Platform m_platform{};
    std::vector<std::string> m_externals{};

    // Visitor scratch state. m_currentList is the InstructionList that
    // emit() pushes into; m_result is set by Expression visit() methods.
    InstructionList* m_currentList{ nullptr };
    OperandPtr m_result{};

    ErrorHandler* m_errorHandler{ nullptr };

    // ScopedEmitTarget (defined in the .cpp) saves/restores m_currentList
    // around a nested emission so visit methods can't leak target state.
    friend struct ScopedEmitTarget;
};

} // namespace Fractal
