// SemanticAnalyzer.cpp
// Visitor-based semantic analyzer implementation. Each visit() method does
// the work that the old analyzeExpressionX / analyzeStatementX / analyze
// DefinitionX functions did, but takes the concrete node by reference (no
// static_pointer_cast) and reports errors through m_ok rather than returning
// bool down a deep call chain.

#include "SemanticAnalyzer.h"

namespace Fractal {

// True if `e` is an l-value the language allows on the LHS of an assignment
// or the receiver of a member access / address-of. We deliberately use
// dynamic_cast here (instead of an enum tag) because the visitor pattern
// already handles dispatch — these checks are just structural predicates.
static bool isLvalue(Expression* e) {
    return dynamic_cast<Identifier*>(e)
        || dynamic_cast<Call*>(e)
        || dynamic_cast<MemberAccess*>(e);
}

static bool isAddressable(Expression* e) {
    return dynamic_cast<Identifier*>(e) || dynamic_cast<MemberAccess*>(e);
}

// "Side-effecting" expressions are ones whose value is allowed to be ignored
// when used as a statement — calls, member-accesses (might be properties),
// and assignments. Everything else triggers an "unused expression" warning.
static bool isSideEffecting(Expression* e) {
    return dynamic_cast<Call*>(e)
        || dynamic_cast<MemberAccess*>(e)
        || dynamic_cast<Assignment*>(e);
}


// -- Symbol lookup -----------------------------------------------------------

bool SemanticAnalyzer::findNameGlobal(const Token& nameToken) {
    return m_globalTable.find(nameToken.value) != m_globalTable.end();
}

int32_t SemanticAnalyzer::findNameLocal(const Token& nameToken) {
    for (int32_t i = (int32_t)m_localStack.size() - 1; i >= 0; i--) {
        if (m_localStack[i].find(nameToken.value) != m_localStack[i].end()) return i;
    }
    return -1;
}

std::string SemanticAnalyzer::createUnique(const std::string& name) {
    return name + "." + std::to_string(m_uniqueIndex++);
}

void SemanticAnalyzer::pushScope() { m_localStack.push_back({}); }
void SemanticAnalyzer::popScope() { m_localStack.pop_back(); }
SymbolTable& SemanticAnalyzer::topScope() { return m_localStack.back(); }

// -- Entry point -------------------------------------------------------------

bool SemanticAnalyzer::analyze(ProgramFile* program) {
    m_program = program;

    // Reset per-run state so the analyzer is safe to reuse across files.
    m_localStack.clear();
    m_loopStack.clear();
    m_currentFunction = nullptr;
    m_uniqueIndex = 0;
    m_loopIndex = 0;
    m_ok = true;
    m_saveMode = false;

    for (auto& definition : m_program->definitions) {
        analyzeDef(definition);
        if (!m_ok) return false;
    }

    pushScope();
    for (auto& statement : m_program->statements) {
        analyze(statement);
        if (!m_ok) return false;
    }
    popScope();

    return m_ok;
}

// -- Dispatch helpers --------------------------------------------------------

void SemanticAnalyzer::analyze(ExpressionPtr& e) {
    if (!m_ok || !e) return;
    e->accept(*this);
}

void SemanticAnalyzer::analyze(StatementPtr& s) {
    if (!m_ok || !s) return;
    s->accept(*this);
}

void SemanticAnalyzer::analyzeDef(DefinitionPtr& d) {
    if (!m_ok || !d) return;
    // Definition derives from Statement; accept(StatementVisitor&) is the
    // only dispatch surface, and StatementVisitor declares visit() methods
    // for every concrete Definition kind.
    d->accept(*this);
}

// -- Expressions: leaves -----------------------------------------------------

void SemanticAnalyzer::visit(IntegerLiteral& node) {
    node.expressionType = std::make_shared<FundamentalType>(BasicType::I32);
}

void SemanticAnalyzer::visit(FloatLiteral& node) {
    node.expressionType = std::make_shared<FundamentalType>(BasicType::F32);
}

void SemanticAnalyzer::visit(StringLiteral& node) {
    node.expressionType = std::make_shared<FundamentalType>(BasicType::String);
}

void SemanticAnalyzer::visit(CharacterLiteral& node) {
    node.expressionType = std::make_shared<FundamentalType>(BasicType::Character);
}

// -- Expressions: composites -------------------------------------------------

void SemanticAnalyzer::visit(ArrayList& node) {
    TypePtr firstType;

    if (!node.elements.empty()) {
        analyze(node.elements[0].expression);
        if (!m_ok) return;
        firstType = node.elements[0].expression->expressionType;
    }

    for (size_t i = 1; i < node.elements.size(); i++) {
        analyze(node.elements[i].expression);
        if (!m_ok) return;
        if (!tryCast(&node.elements[i].expression, firstType)) {
            m_errorHandler->reportError(
                {"Cannot insert element of type '"
                     + node.elements[i].expression->expressionType->typeName()
                     + "' to array which holds elements of type '" + firstType->typeName() + "'",
                 node.elements[i].pos});
            m_ok = false;
            return;
        }
    }

    node.expressionType = std::make_shared<ArrayType>(firstType);
    node.elementType = firstType;
}

void SemanticAnalyzer::visit(BinaryOperation& node) {
    analyze(node.left);
    analyze(node.right);
    if (!m_ok) return;

    if (!handleTypeConversion(&node.left, &node.right)) {
        m_errorHandler->reportError(
            {"Cannot operate between '" + node.right->expressionType->typeName()
                 + "' and '" + node.left->expressionType->typeName() + "' types",
             node.operatorToken.position});
        m_ok = false;
        return;
    }

    node.expressionType = node.left->expressionType;
}

void SemanticAnalyzer::visit(UnaryOperation& node) {
    analyze(node.expression);
    if (!m_ok) return;
    node.expressionType = node.expression->expressionType;
}

void SemanticAnalyzer::visit(Identifier& node) {
    int32_t index = findNameLocal(node.idToken);
    if (index > -1) {
        node.expressionType = m_localStack[index][node.idToken.value].type;
        node.idToken.value = m_localStack[index][node.idToken.value].name;
        return;
    }

    if (!findNameGlobal(node.idToken)) {
        m_errorHandler->reportError(
            {"Undefined name '" + node.idToken.value + "'", node.idToken.position});
        m_ok = false;
        return;
    }
    node.expressionType = m_globalTable[node.idToken.value].type;
}

bool SemanticAnalyzer::compareArgsToParams(const std::vector<TypePtr>& paramList,
                                            Call& call) {
    ArgumentList& argList = call.argumentList;
    if (paramList.size() != argList.size()) {
        m_errorHandler->reportError(
            {"Expected " + std::to_string(paramList.size()) + " arguments in '"
                 + call.funcToken.value + "' call, but got " + std::to_string(argList.size()),
             call.funcToken.position});
        return false;
    }
    for (size_t i = 0; i < paramList.size(); i++) {
        if (!tryCast(&argList[i]->expression, paramList[i])) {
            m_errorHandler->reportError(
                {"Expected argument type '" + paramList[i]->typeName() + "', got '"
                     + argList[i]->expression->expressionType->typeName() + "'",
                 call.funcToken.position});
            return false;
        }
    }
    return true;
}

void SemanticAnalyzer::visit(Call& node) {
    std::shared_ptr<const FunctionType> funcType;

    int32_t index = findNameLocal(node.funcToken);
    if (index > -1) {
        SymbolEntry& symbol = m_localStack[index][node.funcToken.value];
        if (symbol.type->typeInfo() != TypeInfo::Function) {
            m_errorHandler->reportError(
                {"Cannot call non-function names", node.funcToken.position});
            m_ok = false;
            return;
        }
        funcType = std::static_pointer_cast<const FunctionType>(symbol.type);
        node.funcToken.value = symbol.name;
        node.expressionType = funcType->returnType;
    } else if (!findNameGlobal(node.funcToken)) {
        m_errorHandler->reportError(
            {"Undefined name '" + node.funcToken.value + "'", node.funcToken.position});
        m_ok = false;
        return;
    } else {
        SymbolEntry& symbol = m_globalTable[node.funcToken.value];
        if (symbol.type->typeInfo() != TypeInfo::Function) {
            m_errorHandler->reportError(
                {"Cannot call non-function names", node.funcToken.position});
            m_ok = false;
            return;
        }
        funcType = std::static_pointer_cast<const FunctionType>(symbol.type);
        node.expressionType = funcType->returnType;
    }

    for (auto& arg : node.argumentList) {
        analyze(arg->expression);
        if (!m_ok) return;
    }

    if (!compareArgsToParams(funcType->parameterTypes, node)) {
        m_ok = false;
    }
}

void SemanticAnalyzer::visit(Assignment& node) {
    // Only lvalues are assignable.
    if (!isLvalue(node.left.get())) {
        m_errorHandler->reportError(
            {"Cannot assign to non-lvalues", node.operatorToken.position});
        m_ok = false;
        return;
    }

    analyze(node.left);
    analyze(node.right);
    if (!m_ok) return;

    if (!tryCast(&node.right, node.left->expressionType)) {
        m_errorHandler->reportError(
            {"Cannot assign expression of type '" + node.right->expressionType->typeName()
                 + "' to variable of type '" + node.left->expressionType->typeName() + "'",
             node.operatorToken.position});
        m_ok = false;
        return;
    }

    node.expressionType = node.left->expressionType;
}

void SemanticAnalyzer::visit(MemberAccess& node) {
    if (!isLvalue(node.left.get())) {
        m_errorHandler->reportError(
            {"Cannot access member of non-lvalues", node.operatorToken.position});
        m_ok = false;
        return;
    }
    if (!isLvalue(node.right.get())) {
        m_errorHandler->reportError(
            {"Non-lvalues are not valid members of lvalues", node.operatorToken.position});
        m_ok = false;
        return;
    }
    if (dynamic_cast<Identifier*>(node.left.get())) {
        analyze(node.left);
    }
}

void SemanticAnalyzer::visit(CastExpression& node) {
    // CastExpression is a marker the analyzer inserts itself; nothing to do
    // beyond ensuring the inner expression is analyzed.
    analyze(node.expr);
}

void SemanticAnalyzer::visit(DereferenceExpression& node) {
    analyze(node.expr);
    if (!m_ok) return;

    if (node.expr->expressionType->typeInfo() != TypeInfo::Pointer) {
        m_errorHandler->reportError(
            {"Expected pointer type for dereferencing, got '"
                 + node.expr->expressionType->typeName() + "'",
             {}});
        m_ok = false;
        return;
    }
    node.expressionType = std::static_pointer_cast<const PointerType>(node.expr->expressionType)
                              ->pointingType;
}

void SemanticAnalyzer::visit(AddressOfExpression& node) {
    analyze(node.expr);
    if (!m_ok) return;

    node.expressionType = std::make_shared<PointerType>(node.expr->expressionType);
    if (!isAddressable(node.expr.get())) {
        m_errorHandler->reportError({"Cannot get address of a non-lvalue", {}});
        m_ok = false;
    }
}

// -- Statements -------------------------------------------------------------

void SemanticAnalyzer::visit(NullStatement& node) { (void)node; }

void SemanticAnalyzer::visit(CompoundStatement& node) {
    pushScope();
    for (auto& s : node.statements) {
        analyze(s);
        if (!m_ok) {
            popScope();
            return;
        }
    }
    popScope();
}

void SemanticAnalyzer::visit(IfStatement& node) {
    analyze(node.condition);
    analyze(node.ifBody);
    if (node.elseBody) analyze(node.elseBody);
}

void SemanticAnalyzer::visit(LoopStatement& node) {
    m_loopStack.push_back(m_loopIndex++);
    analyze(node.loopBody);
    m_loopStack.pop_back();
}

void SemanticAnalyzer::visit(WhileStatement& node) {
    m_loopStack.push_back(m_loopIndex++);
    analyze(node.condition);
    analyze(node.loopBody);
    m_loopStack.pop_back();
}

void SemanticAnalyzer::visit(BreakStatement& node) {
    if (m_loopStack.empty()) {
        m_errorHandler->reportError(
            {"Cannot use break outside of a loop", node.token.position});
        m_ok = false;
        return;
    }
    node.loopIndex = m_loopStack.back();
}

void SemanticAnalyzer::visit(ContinueStatement& node) {
    if (m_loopStack.empty()) {
        m_errorHandler->reportError(
            {"Cannot use continue outside of a loop", node.token.position});
        m_ok = false;
        return;
    }
    node.loopIndex = m_loopStack.back();
}

void SemanticAnalyzer::visit(ExpressionStatement& node) {
    if (!isSideEffecting(node.expression.get())) {
        m_errorHandler->reportWarning({"Unused expression", node.expressionPos});
    }
    analyze(node.expression);
}

void SemanticAnalyzer::visit(ReturnStatement& node) {
    if (!m_currentFunction) {
        m_errorHandler->reportError(
            {"Cannot use return outside of a function body", node.token.position});
        m_ok = false;
        return;
    }

    analyze(node.expression);
    if (!m_ok) return;

    if (!tryCast(&node.expression, m_currentFunction->returnType)) {
        m_errorHandler->reportError(
            {"Cannot return type '" + node.expression->expressionType->typeName()
                 + "' from a function which returns type '"
                 + m_currentFunction->returnType->typeName() + "'",
             node.token.position});
        m_ok = false;
    }
}

// -- Definitions -------------------------------------------------------------

bool SemanticAnalyzer::analyzeParameters(const ParameterList& paramList) {
    std::vector<std::string> seen;
    for (const auto& parameter : paramList) {
        if (findNameGlobal(parameter->nameToken))
            m_errorHandler->reportWarning(
                {"Parameter '" + parameter->nameToken.value + "' shadows a global name",
                 parameter->nameToken.position});
        if (std::find(seen.begin(), seen.end(), parameter->nameToken.value) != seen.end()) {
            m_errorHandler->reportError(
                {"Parameter '" + parameter->nameToken.value + "' is already defined",
                 parameter->nameToken.position});
            return false;
        }
        seen.push_back(parameter->nameToken.value);
        std::string newName = createUnique(parameter->nameToken.value);
        topScope()[parameter->nameToken.value] = {newName, parameter->type};
        parameter->nameToken.value = newName;
    }
    return true;
}

void SemanticAnalyzer::visit(FunctionDefinition& node) {
    pushScope();

    if (findNameGlobal(node.nameToken)) {
        m_errorHandler->reportError(
            {"Function '" + node.nameToken.value + "' is already defined",
             node.nameToken.position});
        m_ok = false;
        popScope();
        return;
    }

    if (!analyzeParameters(node.parameterList)) {
        m_ok = false;
        popScope();
        return;
    }

    std::vector<TypePtr> parameterTypes;
    parameterTypes.reserve(node.parameterList.size());
    for (auto& param : node.parameterList) parameterTypes.push_back(param->type);

    m_currentFunction = std::make_shared<FunctionType>(node.returnType, parameterTypes);
    m_globalTable[node.nameToken.value] =
        SymbolEntry{node.nameToken.value, m_currentFunction};

    if (m_saveMode) {
        popScope();
        return;
    }

    analyze(node.functionBody);
    m_currentFunction = nullptr;
    popScope();
}

void SemanticAnalyzer::visit(VariableDefinition& node) {
    if (node.isGlobal) {
        if (findNameGlobal(node.nameToken)) {
            m_errorHandler->reportError(
                {"Variable '" + node.nameToken.value + "' is already defined globally",
                 node.nameToken.position});
            m_ok = false;
            return;
        }
        m_globalTable[node.nameToken.value] =
            SymbolEntry{node.nameToken.value, node.variableType};
    } else {
        if (findNameLocal(node.nameToken) > -1) {
            m_errorHandler->reportError(
                {"Variable '" + node.nameToken.value + "' is already defined in local scope",
                 node.nameToken.position});
            m_ok = false;
            return;
        }
        topScope()[node.nameToken.value] =
            SymbolEntry{node.nameToken.value, node.variableType};
    }

    if (node.initializer) {
        analyze(node.initializer);
        if (!m_ok) return;

        if (node.variableType->typeInfo() == TypeInfo::Fundamental
            && std::static_pointer_cast<const FundamentalType>(node.variableType)->type
                   == BasicType::None) {
            node.variableType = node.initializer->expressionType;
        } else if (!tryCast(&node.initializer, node.variableType)) {
            m_errorHandler->reportError(
                {"Initializer Expression does not match the variable's type",
                 node.nameToken.position});
            m_ok = false;
        }
    }
}

void SemanticAnalyzer::visit(ClassDefinition& node) {
    m_userDefinedTypes.push_back(node.className);
}

void SemanticAnalyzer::visit(DecoratedDefinition& node) {
    if (node.decorator == Decorator::External) {
        analyzeDef(node.definition);
    }
}

// -- Type conversion helpers (unchanged) ------------------------------------

bool SemanticAnalyzer::handleTypeConversion(ExpressionPtr* a, ExpressionPtr* b) {
    if (sameType((*a)->expressionType, (*b)->expressionType)) return true;
    Size x = isNumType((*a)->expressionType);
    Size y = isNumType((*b)->expressionType);
    if ((bool)x && (bool)y) {
        bool xGreater = (int)x > (int)y;
        cast(xGreater ? b : a, xGreater ? (*a)->expressionType : (*b)->expressionType);
        return true;
    }
    return false;
}

bool SemanticAnalyzer::tryCast(ExpressionPtr* original, TypePtr target) {
    if (sameType((*original)->expressionType, target)) return true;
    Size x = isNumType((*original)->expressionType);
    Size y = isNumType(target);
    if ((bool)x && (bool)y) {
        cast(original, target);
        return true;
    }
    return false;
}

void SemanticAnalyzer::cast(ExpressionPtr* original, TypePtr target) {
    auto castExpr = std::make_unique<CastExpression>(std::move(*original), target);
    castExpr->expressionType = target;
    *original = std::move(castExpr);
}

} // namespace Fractal
