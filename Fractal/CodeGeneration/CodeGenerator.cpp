// CodeGenerator.cpp
// Visitor-based IR code generator. Each visit() emits instructions into
// m_currentList; expression visit() methods also stash their result on
// m_result for the parent to read via generate().

#include "CodeGenerator.h"

namespace Fractal {

    // -- Type sizing ------------------------------------------------------------

    static Size getTypeSize(TypePtr type) {
        switch (type->typeInfo()) {
        case TypeInfo::Fundamental: {
            auto fundamentalType = std::static_pointer_cast<const FundamentalType>(type);
            switch (fundamentalType->type) {
            case BasicType::I32:
            case BasicType::U32:
            case BasicType::F32:
                return Size::DWord;
            case BasicType::I64:
            case BasicType::U64:
            case BasicType::F64:
                return Size::QWord;
            default:
                return Size::DWord;
            }
        }
        case TypeInfo::Pointer:
        case TypeInfo::Array:
        case TypeInfo::Function:
            return Size::QWord;
        default:
            return Size::DWord;
        }
    }

    // Returns the byte-size of an expression's value as the SemanticAnalyzer
    // determined it. Falls back to DWord if the expression has no resolved
    // type, which only happens before the analyzer has run (e.g. when codegen
    // is invoked on partial AST in tests).
    static Size resultSize(const Expression& e) {
        return e.expressionType ? getTypeSize(e.expressionType) : Size::DWord;
    }

    static ComparisonType getComparisonType(TokenType tokType) {
        switch (tokType) {
        case EQUAL_EQUAL:
            return ComparisonType::Equal;
        case BANG_EQUAL:
            return ComparisonType::NotEqual;
        case GREATER:
            return ComparisonType::Greater;
        case GREATER_EQUAL:
            return ComparisonType::GreaterEqual;
        case LESS:
            return ComparisonType::Less;
        case LESS_EQUAL:
            return ComparisonType::LessEqual;
        default:
            return ComparisonType::None;
        }
    }

    // -- Entry point ------------------------------------------------------------

    const InstructionList& CodeGenerator::generate(const ProgramFile& program, Platform platform) {
        // m_program is borrowed for the duration of this call only; the AST is
        // owned by the caller via unique_ptr, so we keep a non-owning pointer.
        m_program = const_cast<ProgramFile*>(&program);
        m_instructions.clear();
        m_platform = platform;
        m_currentList = &m_instructions;

        for (auto& definition : program.definitions) {
            definition->accept(*this);
        }

        // Two-pass legalization: validateMove may insert `movsx` shimmed by an
        // extra `mov` that itself violates size-match rules. The second pass
        // resolves the inserted instructions. This is fragile (a future shim
        // requiring three passes would silently break) and the right fix is
        // to make the move factory emit legal instructions directly, or to
        // run a proper fixed-point legalization. Tracked as a TODO.
        validateInstructions(&m_instructions);
        validateInstructions(&m_instructions);

        return m_instructions;
    }

    // -- Generate helper --------------------------------------------------------

    // RAII guard that redirects emit() to a new InstructionList for the
    // lifetime of the guard, then restores the previous target. Used so
    // visit(FunctionDefinition&) (and any future nested-emission visitor)
    // can't forget to put m_currentList back even on early return / exception.
    struct ScopedEmitTarget {
        CodeGenerator& gen;
        InstructionList* previous;
        ScopedEmitTarget(CodeGenerator& g, InstructionList* newList)
            : gen{g}, previous{g.m_currentList} {
            g.m_currentList = newList;
        }
        ScopedEmitTarget(const ScopedEmitTarget&) = delete;
        ScopedEmitTarget& operator=(const ScopedEmitTarget&) = delete;
        ~ScopedEmitTarget() { gen.m_currentList = previous; }
    };

    // Reports an "unimplemented codegen for X" error and clears m_result.
    // Used as the body of every visit() that doesn't lower yet, so a missing
    // feature surfaces as a hard error instead of silently corrupting output.
    void CodeGenerator::notImplemented(const std::string& nodeName, const Position& pos) {
        if (m_errorHandler)
            m_errorHandler->reportError(
                {"Code generation for '" + nodeName + "' is not implemented yet", pos});
        m_result = nullptr;
    }

    OperandPtr CodeGenerator::generate(Expression* expression) {
        if (!expression)
            return nullptr;
        OperandPtr saved = std::move(m_result);
        m_result.reset();
        expression->accept(*this);
        OperandPtr result = std::move(m_result);
        m_result = std::move(saved);
        return result;
    }

    // -- Definitions ------------------------------------------------------------

    void CodeGenerator::visit(FunctionDefinition& node) {
        // Save current local variable map
        auto savedVars = std::move(m_localVarMap);
        m_localVarMap.clear();

        m_currentStackIndex = 0;
        auto funcInstr =
            std::make_unique<FunctionDefInstruction>(node.nameToken.value, InstructionList{});

        std::vector<Register> argumentRegisters =
            (m_platform == Platform::Win)
                ? std::vector<Register>{Register::CX, Register::DX, Register::R8, Register::R9}
                : std::vector<Register>{Register::DI, Register::SI, Register::DX,
                                        Register::CX, Register::R8, Register::R9};

        bool addToStack = node.parameterList.size() > argumentRegisters.size();
        size_t inRegs = addToStack ? argumentRegisters.size() : node.parameterList.size();

        // Redirect emit() to write into the function's own instruction list
        // for the scoped block below; ScopedEmitTarget restores m_currentList
        // when the block exits. This keeps the rest of the function readable
        // without leaking the redirect into the final emit(std::move(funcInstr)).
        {
        ScopedEmitTarget _target{*this, &funcInstr->instructions};

        for (size_t i = 0; i < inRegs; i++) {
            Size paramSize = getTypeSize(node.parameterList[i]->type);
            OperandPtr stackParam =
                std::make_shared<TempOperand>(allocateStack(paramSize), paramSize);
            emit(move(reg(argumentRegisters[i], paramSize), stackParam));
            m_localVarMap[node.parameterList[i]->nameToken.value] = stackParam;
        }

        if (addToStack) {
            for (int64_t i = (int64_t)node.parameterList.size() - 1;
                 i >= (int64_t)argumentRegisters.size(); i--) {
                m_localVarMap[node.parameterList[i]->nameToken.value] =
                    std::make_shared<TempOperand>(-(i - (int64_t)argumentRegisters.size() + 2) * 8,
                                                  getTypeSize(node.parameterList[i]->type));
            }
        }

        if (node.functionBody)
            node.functionBody->accept(*this);

        funcInstr->stackAlloc = m_currentStackIndex;

        // Implicit `return 0` if control flow falls off the end.
        emit(move(intConst(0), reg(Register::AX, Size::DWord)));
        emit(std::make_unique<ReturnInstruction>());
        } // ScopedEmitTarget destructor restores m_currentList to the outer list

        // Now emit the assembled function instruction into the program's list.
        emit(std::move(funcInstr));

        // Restore local variable map
        m_localVarMap = std::move(savedVars);
    }

    void CodeGenerator::visit(VariableDefinition& node) {
        Size varSize = getTypeSize(node.variableType);
        OperandPtr varPtr = std::make_shared<TempOperand>(allocateStack(varSize), varSize);
        m_localVarMap[node.nameToken.value] = varPtr;

        if (node.initializer) {
            OperandPtr init = generate(node.initializer.get());
            if (init)
                emit(move(init, varPtr));
        }
    }

    void CodeGenerator::visit(ClassDefinition& node) {
        (void)node; // class bodies are not lowered yet
    }

    void CodeGenerator::visit(DecoratedDefinition& node) {
        if (node.decorator == Decorator::External) {
            m_externals.push_back(node.definition->getName());
        }
    }

    // -- Statements -------------------------------------------------------------

    void CodeGenerator::visit(NullStatement& node) {
        (void)node;
    }

    void CodeGenerator::visit(CompoundStatement& node) {
        for (auto& s : node.statements)
            if (s)
                s->accept(*this);
    }

    void CodeGenerator::visit(ExpressionStatement& node) {
        generate(node.expression.get());
    }

    void CodeGenerator::visit(ReturnStatement& node) {
        OperandPtr result = generate(node.expression.get());
        if (result) {
            // Use the expression's own type rather than a hardcoded DWord so
            // i64/pointer returns end up in RAX, not just EAX.
            Size sz = node.expression ? resultSize(*node.expression) : Size::DWord;
            emit(move(result, reg(Register::AX, sz)));
        }
        emit(std::make_unique<ReturnInstruction>());
    }

    void CodeGenerator::visit(IfStatement& node) {
        uint64_t index = generateIfIndex();
        std::string falseLabel = ".IF" + std::to_string(index);
        std::string endLabel = ".IE" + std::to_string(index);

        if (!node.elseBody)
            falseLabel = endLabel;

        OperandPtr cond = generate(node.condition.get());
        emit(cmp(cond, intConst(0)));
        emit(jmp(falseLabel, ComparisonType::Equal));
        if (node.ifBody)
            node.ifBody->accept(*this);

        if (node.elseBody) {
            emit(jmp(endLabel, ComparisonType::None));
            emit(label(falseLabel));
            node.elseBody->accept(*this);
        }
        emit(label(endLabel));
    }

    void CodeGenerator::visit(LoopStatement& node) {
        uint64_t index = generateIfIndex();
        std::string startLabel = ".LS" + std::to_string(index);
        std::string exitLabel = ".LE" + std::to_string(index);

        m_loopStack.push_back({startLabel, exitLabel});

        emit(label(startLabel));
        if (node.loopBody)
            node.loopBody->accept(*this);
        emit(jmp(startLabel, ComparisonType::None));
        emit(label(exitLabel));

        m_loopStack.pop_back();
    }

    void CodeGenerator::visit(WhileStatement& node) {
        uint64_t index = generateIfIndex();
        std::string startLabel = ".LS" + std::to_string(index);
        std::string exitLabel = ".LE" + std::to_string(index);

        m_loopStack.push_back({startLabel, exitLabel});

        emit(label(startLabel));
        OperandPtr cond = generate(node.condition.get());
        emit(cmp(cond, intConst(0)));
        emit(jmp(exitLabel, ComparisonType::Equal));
        if (node.loopBody)
            node.loopBody->accept(*this);
        emit(jmp(startLabel, ComparisonType::None));
        emit(label(exitLabel));

        m_loopStack.pop_back();
    }

    void CodeGenerator::visit(BreakStatement& node) {
        (void)node;
        if (m_loopStack.empty())
            return;
        emit(jmp(m_loopStack.back().exitLabel, ComparisonType::None));
    }

    void CodeGenerator::visit(ContinueStatement& node) {
        (void)node;
        if (m_loopStack.empty())
            return;
        emit(jmp(m_loopStack.back().startLabel, ComparisonType::None));
    }

    // -- Expressions: leaves ----------------------------------------------------

    void CodeGenerator::visit(IntegerLiteral& node) {
        m_result = intConst(node.value);
    }

    void CodeGenerator::visit(FloatLiteral& node) {
        notImplemented("FloatLiteral", node.position);
    }

    void CodeGenerator::visit(StringLiteral& node) {
        notImplemented("StringLiteral", node.position);
    }

    void CodeGenerator::visit(CharacterLiteral& node) {
        notImplemented("CharacterLiteral", node.position);
    }

    void CodeGenerator::visit(ArrayList& node) {
        (void)node;
        notImplemented("ArrayList", {});
    }

    void CodeGenerator::visit(Identifier& node) {
        m_result = m_localVarMap[node.idToken.value];
    }

    // -- Expressions: composites ------------------------------------------------

    void CodeGenerator::visit(UnaryOperation& node) {
        Size sz = resultSize(node);
        auto destination = std::make_shared<TempOperand>(allocateStack(sz), sz);
        OperandPtr inner = generate(node.expression.get());
        if (inner)
            emit(move(inner, destination));

        switch (node.operatorToken.type) {
        case MINUS:
            emit(negate(destination));
            break;
        case TILDE:
            emit(bitwiseNot(destination));
            break;
        default:
            break;
        }

        m_result = destination;
    }

    void CodeGenerator::visit(BinaryOperation& node) {
        switch (node.operatorToken.type) {
        case PLUS:
        case MINUS:
        case STAR:
            m_result = arithmetic(node);
            return;
        case SLASH:
            m_result = idiv(node);
            return;
        case LESS:
        case LESS_EQUAL:
        case GREATER:
        case GREATER_EQUAL:
        case EQUAL_EQUAL:
        case BANG_EQUAL:
            m_result = relational(node);
            return;
        case OR:
        case AND:
            m_result = logical(node);
            return;
        default:
            notImplemented(
                "BinaryOperation '" + node.operatorToken.value + "'",
                node.operatorToken.position);
            return;
        }
    }

    OperandPtr CodeGenerator::arithmetic(BinaryOperation& node) {
        Size sz = resultSize(node);
        auto destination = std::make_shared<TempOperand>(allocateStack(sz), sz);
        OperandPtr leftOp = generate(node.left.get());
        if (leftOp)
            emit(move(leftOp, destination));

        switch (node.operatorToken.type) {
        case PLUS:
            emit(add(destination, generate(node.right.get())));
            break;
        case MINUS:
            emit(sub(destination, generate(node.right.get())));
            break;
        case STAR:
            emit(mul(destination, generate(node.right.get())));
            break;
        default:
            break;
        }
        return destination;
    }

    OperandPtr CodeGenerator::relational(BinaryOperation& node) {
        auto destination = std::make_shared<TempOperand>(allocateStack(Size::DWord), Size::Byte);
        ComparisonType type = getComparisonType(node.operatorToken.type);
        OperandPtr leftOp = generate(node.left.get());
        OperandPtr rightOp = generate(node.right.get());
        emit(cmp(leftOp, rightOp));
        emit(set(destination, type));
        return destination;
    }

    OperandPtr CodeGenerator::logical(BinaryOperation& node) {
        auto destination = std::make_shared<TempOperand>(allocateStack(Size::DWord), Size::DWord);

        uint64_t index = generateComparisonIndex();
        std::string falseLabel = ".CF" + std::to_string(index);
        std::string trueLabel = ".CT" + std::to_string(index);
        std::string endLabel = ".CE" + std::to_string(index);

        if (node.operatorToken.type == AND) {
            OperandPtr a = generate(node.left.get());
            emit(cmp(a, intConst(0)));
            emit(jmp(falseLabel, ComparisonType::Equal));
            OperandPtr b = generate(node.right.get());
            emit(cmp(b, intConst(0)));
            emit(jmp(falseLabel, ComparisonType::Equal));

            emit(move(intConst(1), destination));
            emit(jmp(endLabel, ComparisonType::None));
            emit(label(falseLabel));
            emit(move(intConst(0), destination));
        } else {
            OperandPtr a = generate(node.left.get());
            emit(cmp(a, intConst(1)));
            emit(jmp(trueLabel, ComparisonType::Equal));
            OperandPtr b = generate(node.right.get());
            emit(cmp(b, intConst(1)));
            emit(jmp(trueLabel, ComparisonType::Equal));

            emit(move(intConst(0), destination));
            emit(jmp(endLabel, ComparisonType::None));
            emit(label(trueLabel));
            emit(move(intConst(1), destination));
        }
        emit(label(endLabel));

        return destination;
    }

    OperandPtr CodeGenerator::idiv(BinaryOperation& node) {
        Size sz = resultSize(node);
        OperandPtr rightOp = generate(node.right.get());
        auto temp = std::make_shared<TempOperand>(allocateStack(sz), sz);
        if (rightOp)
            emit(move(rightOp, temp));
        OperandPtr leftOp = generate(node.left.get());
        if (leftOp)
            emit(move(leftOp, reg(Register::AX, sz)));
        emit(std::make_unique<CdqInstruction>());
        emit(std::make_unique<DivInstruction>(temp));
        return reg(Register::AX, sz);
    }

    void CodeGenerator::visit(Assignment& node) {
        OperandPtr temp = generate(node.right.get());
        OperandPtr var = generate(node.left.get());
        if (temp && var)
            emit(move(temp, var));
        m_result = var;
    }

    void CodeGenerator::visit(Call& node) {
        int stackPadding = (m_platform == Platform::Win ? 32 : 0);
        if (node.argumentList.size() % 2 == 0)
            stackPadding += 8;

        std::vector<Register> argumentRegisters =
            (m_platform == Platform::Win)
                ? std::vector<Register>{Register::CX, Register::DX, Register::R8, Register::R9}
                : std::vector<Register>{Register::DI, Register::SI, Register::DX,
                                        Register::CX, Register::R8, Register::R9};

        emit(sub(reg(Register::SP, Size::QWord), intConst(stackPadding)));

        bool addToStack = node.argumentList.size() > argumentRegisters.size();
        size_t inRegs = addToStack ? argumentRegisters.size() : node.argumentList.size();

        for (size_t i = 0; i < inRegs; i++) {
            OperandPtr argOp = generate(node.argumentList[i]->expression.get());
            if (argOp)
                emit(move(argOp, reg(argumentRegisters[i], Size::DWord)));
        }

        uint32_t stackArgs = 0;
        if (addToStack) {
            for (int64_t i = (int64_t)node.argumentList.size() - 1;
                 i >= (int64_t)argumentRegisters.size(); i--) {
                OperandPtr argOp = generate(node.argumentList[i]->expression.get());
                if (argOp)
                    emit(push(argOp));
                stackArgs++;
            }
        }

        emit(call(node.funcToken.value));
        emit(add(reg(Register::SP, Size::QWord), intConst(8 * stackArgs + stackPadding)));

        m_result = reg(Register::AX, Size::DWord);
    }

    void CodeGenerator::visit(MemberAccess& node) {
        notImplemented("MemberAccess", node.operatorToken.position);
    }

    void CodeGenerator::visit(CastExpression& node) {
        Size typeSize = getTypeSize(node.target);
        OperandPtr temp = std::make_shared<TempOperand>(allocateStack(typeSize), typeSize);
        OperandPtr inner = generate(node.expr.get());
        if (inner) {
            inner->setSize(typeSize);
            emit(move(inner, temp));
        }
        m_result = temp;
    }

    void CodeGenerator::visit(AddressOfExpression& node) {
        (void)node;
        notImplemented("AddressOfExpression", {});
    }

    void CodeGenerator::visit(DereferenceExpression& node) {
        (void)node;
        notImplemented("DereferenceExpression", {});
    }

    // -- Instruction factories --------------------------------------------------

    InstructionPtr CodeGenerator::move(OperandPtr source, OperandPtr destination) {
        auto instr = std::make_unique<MoveInstruction>(source, destination);
        if (destination)
            instr->destSize = destination->getSize();
        if (source)
            instr->srcSize = source->getSize();
        return instr;
    }

    InstructionPtr CodeGenerator::negate(OperandPtr source) {
        return std::make_unique<NegateInstruction>(source);
    }

    InstructionPtr CodeGenerator::bitwiseNot(OperandPtr source) {
        return std::make_unique<BitwiseNotInstruction>(source);
    }

    InstructionPtr CodeGenerator::add(OperandPtr destination, OperandPtr other) {
        return std::make_unique<AddInstruction>(destination, other);
    }

    InstructionPtr CodeGenerator::sub(OperandPtr destination, OperandPtr other) {
        return std::make_unique<SubtractInstruction>(destination, other);
    }

    InstructionPtr CodeGenerator::mul(OperandPtr destination, OperandPtr other) {
        return std::make_unique<MultiplyInstruction>(destination, other);
    }

    InstructionPtr CodeGenerator::cmp(OperandPtr left, OperandPtr right) {
        return std::make_unique<CompareInstruction>(left, right);
    }

    InstructionPtr CodeGenerator::set(OperandPtr operand, ComparisonType type) {
        return std::make_unique<SetInstruction>(operand, type);
    }

    InstructionPtr CodeGenerator::jmp(const std::string& label, ComparisonType type) {
        return std::make_unique<JumpInstruction>(label, type);
    }

    InstructionPtr CodeGenerator::call(const std::string& func) {
        return std::make_unique<CallInstruction>(func);
    }

    InstructionPtr CodeGenerator::push(OperandPtr src) {
        return std::make_unique<PushInstruction>(src);
    }

    OperandPtr CodeGenerator::reg(Register register_, Size size) {
        return std::make_shared<RegisterOperand>(register_, size);
    }

    OperandPtr CodeGenerator::intConst(int64_t integer) {
        return std::make_shared<IntegerConstant>(integer);
    }

    InstructionPtr CodeGenerator::label(const std::string& name) {
        return std::make_unique<Label>(name);
    }

    int64_t CodeGenerator::allocateStack(Size size) {
        m_currentStackIndex += (int)size;
        return m_currentStackIndex;
    }

    uint64_t CodeGenerator::generateComparisonIndex() {
        return ++m_currentComparisonIndex;
    }
    uint64_t CodeGenerator::generateIfIndex() {
        return ++m_currentIfIndex;
    }
    uint64_t CodeGenerator::generateLoopIndex() {
        return ++m_currentLoopIndex;
    }

    // -- Validation pass --------------------------------------------------------

    void CodeGenerator::validateInstructions(InstructionList* instructions) {
        for (size_t i = 0; i < instructions->size(); i++) {
            switch ((*instructions)[i]->getType()) {
            case InstructionType::FunctionDefinition:
                validateFunction((*instructions)[i].get());
                break;
            case InstructionType::Move:
                validateMove(instructions, i);
                break;
            case InstructionType::Add:
                validateAdd(instructions, i);
                break;
            case InstructionType::Subtract:
                validateSub(instructions, i);
                break;
            case InstructionType::Multiply:
                validateMul(instructions, i);
                break;
            case InstructionType::Divide:
                validateDiv(instructions, i);
                break;
            case InstructionType::Compare:
                validateCmp(instructions, i);
                break;
            case InstructionType::Push:
                validatePush(instructions, i);
                break;
            default:
                break;
            }
        }
    }

    void CodeGenerator::validateFunction(Instruction* instruction) {
        auto* func = static_cast<FunctionDefInstruction*>(instruction);
        validateInstructions(&func->instructions);
    }

    static bool isTemp(OperandPtr operand) {
        return operand && operand->getType() == OperandType::Temp;
    }

    void CodeGenerator::validateMoveOperands(InstructionList* instructions, size_t i,
                                             OperandPtr source, OperandPtr* destination) {
        if (isTemp(source) && isTemp(*destination)) {
            OperandPtr scratchReg = reg(Register::R10, source->getSize());
            OperandPtr oldDestination = *destination;
            *destination = scratchReg;
            instructions->emplace(instructions->begin() + i + 1, move(scratchReg, oldDestination));
        }
    }

    void CodeGenerator::validateBinOperands(InstructionList* instructions, size_t i,
                                            OperandPtr source, OperandPtr* other) {
        if (isTemp(source) && isTemp(*other)) {
            OperandPtr scratchReg = reg(Register::R10);
            OperandPtr oldOther = *other;
            *other = scratchReg;
            instructions->emplace(instructions->begin() + i, move(oldOther, scratchReg));
        }
    }

    void CodeGenerator::validateMove(InstructionList* instructions, size_t i) {
        auto* moveInstruction = static_cast<MoveInstruction*>((*instructions)[i].get());
        if (!moveInstruction->source || !moveInstruction->destination)
            return;

        if (moveInstruction->destSize < moveInstruction->srcSize &&
            moveInstruction->source->getType() != OperandType::IntegerConstant) {
            moveInstruction->srcSize = moveInstruction->destSize;
        }
        if (moveInstruction->destSize > moveInstruction->srcSize &&
            moveInstruction->source->getType() != OperandType::IntegerConstant) {
            OperandPtr scratchReg = reg(Register::AX, moveInstruction->destination->getSize());
            OperandPtr oldDest = moveInstruction->destination;
            moveInstruction->destination = scratchReg;
            moveInstruction->signExtend = true;
            instructions->emplace(instructions->begin() + i + 1, move(scratchReg, oldDest));
        } else {
            validateMoveOperands(instructions, i, moveInstruction->source,
                                 &moveInstruction->destination);
        }
    }

    void CodeGenerator::validateAdd(InstructionList* instructions, size_t i) {
        auto* addInstruction = static_cast<AddInstruction*>((*instructions)[i].get());
        validateBinOperands(instructions, i, addInstruction->destination, &addInstruction->other);
    }

    void CodeGenerator::validateSub(InstructionList* instructions, size_t i) {
        auto* subInstruction = static_cast<SubtractInstruction*>((*instructions)[i].get());
        validateBinOperands(instructions, i, subInstruction->destination, &subInstruction->other);
    }

    void CodeGenerator::validateMul(InstructionList* instructions, size_t i) {
        auto* mulInstruction = static_cast<MultiplyInstruction*>((*instructions)[i].get());
        if (isTemp(mulInstruction->destination)) {
            OperandPtr scratchReg = reg(Register::R11);
            OperandPtr oldDestination = mulInstruction->destination;
            mulInstruction->destination = scratchReg;
            instructions->emplace(instructions->begin() + i, move(oldDestination, scratchReg));
            instructions->emplace(instructions->begin() + i + 2, move(scratchReg, oldDestination));
        }
    }

    void CodeGenerator::validateDiv(InstructionList* instructions, size_t i) {
        (void)instructions;
        (void)i;
    }

    void CodeGenerator::validateCmp(InstructionList* instructions, size_t i) {
        auto* cmpInstruction = static_cast<CompareInstruction*>((*instructions)[i].get());
        if (cmpInstruction->left->getType() == OperandType::IntegerConstant ||
            isTemp(cmpInstruction->left)) {
            OperandPtr scratchReg = reg(Register::AX);
            OperandPtr oldLeft = cmpInstruction->left;
            cmpInstruction->left = scratchReg;
            instructions->emplace(instructions->begin() + i, move(oldLeft, scratchReg));
        }
    }

    void CodeGenerator::validatePush(InstructionList* instructions, size_t i) {
        auto* pushInstruction = static_cast<PushInstruction*>((*instructions)[i].get());
        if (pushInstruction->src->getType() != OperandType::IntegerConstant &&
            pushInstruction->src->getSize() != Size::QWord) {
            OperandPtr scratchReg = reg(Register::AX, Size::QWord);
            instructions->emplace(instructions->begin() + i,
                                  move(pushInstruction->src, scratchReg));
            pushInstruction->src = scratchReg;
        }
    }

} // namespace Fractal
