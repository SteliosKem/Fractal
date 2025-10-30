// CodeGenerator.h
// Contains the CodeGenerator method implementations
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "CodeGenerator.h"

namespace Fractal {
	Size getTypeSize(TypePtr type) {
		switch (type->typeInfo()) {
			case TypeInfo::Fundamental: {
				std::shared_ptr<FundamentalType> fundamentalType = static_pointer_cast<FundamentalType>(type);
				switch (fundamentalType->type) {
				case BasicType::I32: return Size::DWord;
				case BasicType::I64: return Size::QWord;
				}
			}
		}
	}

	const InstructionList& CodeGenerator::generate(const ProgramFile& program, Platform platform) {
		m_program = program;
		m_instructions = {};
		m_platform = platform;

		for (auto definition : program.definitions)
			generateDefinition(definition, &m_instructions);

		std::shared_ptr<FunctionDefInstruction> mainFunc = std::make_shared<FunctionDefInstruction>("main", InstructionList{});
		for (auto statement : program.statements)
			generateStatement(statement, &mainFunc->instructions);

		validateInstructions(&m_instructions);
		validateInstructions(&m_instructions);

		return m_instructions;
	}

	void CodeGenerator::generateDefinition(DefinitionPtr definition, InstructionList* instructions) {
		switch (definition->getType()) {
			case NodeType::FunctionDefinition: generateFunctionDefinition(definition, instructions);
			case NodeType::DecoratedDefinition: generateDecoratedDefinition(definition, instructions);
			default: return;
		}
	}

	void CodeGenerator::generateDecoratedDefinition(DefinitionPtr definition, InstructionList* instructions) {
		std::shared_ptr<DecoratedDefinition> dec = static_pointer_cast<DecoratedDefinition>(definition);
		if (dec->decorator == Decorator::External) {
			m_externals.push_back(dec->definition->getName());
			return;
		}
	}

	void CodeGenerator::generateFunctionDefinition(DefinitionPtr definition, InstructionList* instructions) {
		m_currentStackIndex = 0;
		std::shared_ptr<FunctionDefinition> func = static_pointer_cast<FunctionDefinition>(definition);
		std::shared_ptr<FunctionDefInstruction> mainFunc = std::make_shared<FunctionDefInstruction>("main", InstructionList{});

		mainFunc->name = func->nameToken.value;

		std::vector<Register> argumentRegisters = (m_platform == Platform::Win)
			? std::vector<Register>{ Register::CX, Register::DX, Register::R8, Register::R9 }
		: std::vector<Register>{ Register::DI, Register::SI, Register::DX, Register::CX, Register::R8, Register::R9 };

		bool addToStack = false;
		size_t m = (func->parameterList.size() > argumentRegisters.size() ? addToStack = true, argumentRegisters.size() : func->parameterList.size());
		for (size_t i = 0; i < m; i++) {
			Size paramSize = getTypeSize(func->parameterList[i]->type);
			OperandPtr stackParam = std::make_shared<TempOperand>(allocateStack(paramSize), paramSize);
			mainFunc->instructions.push_back(move(reg(argumentRegisters[i], paramSize), stackParam));
			m_localVarMap[func->parameterList[i]->nameToken.value] = stackParam;
		}

		uint32_t stackArgs = 0;

		if (addToStack) {
			for (size_t i = func->parameterList.size() - 1; i >= argumentRegisters.size(); i--)
				m_localVarMap[func->parameterList[i]->nameToken.value] = std::make_shared<TempOperand>(-(i - argumentRegisters.size() + 2) * 8, getTypeSize(func->parameterList[i]->type));
		}

		generateStatement(func->functionBody, &mainFunc->instructions);

		instructions->push_back(mainFunc);
		mainFunc->stackAlloc = m_currentStackIndex;

		mainFunc->instructions.push_back(move(intConst(0), reg(Register::AX, Size::DWord)));
		mainFunc->instructions.push_back(std::make_shared<ReturnInstruction>());
	}

	void CodeGenerator::generateVariableDefinition(StatementPtr definition, InstructionList* instructions) {
		std::cout << "mm";
		std::shared_ptr<VariableDefinition> varDef = static_pointer_cast<VariableDefinition>(definition);
		Size varSize = getTypeSize(varDef->variableType);
		OperandPtr varPtr = std::make_shared<TempOperand>(allocateStack(varSize), varSize);
		m_localVarMap[varDef->nameToken.value] = varPtr;
		if (varDef->initializer)
			instructions->push_back(move(generateExpression(varDef->initializer, instructions), varPtr));
	}

	InstructionPtr CodeGenerator::label(const std::string& name) {
		return std::make_shared<Label>(name);
	}

	void CodeGenerator::generateStatement(StatementPtr statement, InstructionList* instructions) {
		switch (statement->getType()) {
			case NodeType::ReturnStatement: generateReturnStatement(statement, instructions); return;
			case NodeType::CompoundStatement: generateCompoundStatement(statement, instructions); return;
			case NodeType::VariableDefinition: generateVariableDefinition(statement, instructions); return;
			case NodeType::ExpressionStatement: generateExpressionStatement(statement, instructions); return;
			case NodeType::IfStatement: generateIfStatement(statement, instructions); return;
			case NodeType::LoopStatement: generateLoopStatement(statement, instructions); return;
			case NodeType::WhileStatement: generateWhileStatement(statement, instructions); return;
			case NodeType::BreakStatement: generateBreakStatement(statement, instructions); return;
			case NodeType::ContinueStatement: generateContinueStatement(statement, instructions); return;
			default: return;
		}
	}

	void CodeGenerator::generateExpressionStatement(StatementPtr statement, InstructionList* instructions) {
		std::shared_ptr<ExpressionStatement> exprStatement = static_pointer_cast<ExpressionStatement>(statement);
		generateExpression(exprStatement->expression, instructions);
	}

	void CodeGenerator::generateCompoundStatement(StatementPtr statement, InstructionList* instructions) {
		std::shared_ptr<CompoundStatement> compoundStatement = static_pointer_cast<CompoundStatement>(statement);

		for (auto statement_ : compoundStatement->statements)
			generateStatement(statement_, instructions);
	}

	void CodeGenerator::generateReturnStatement(StatementPtr statement, InstructionList* instructions) {
		std::shared_ptr<ReturnStatement> returnStatement = static_pointer_cast<ReturnStatement>(statement);

		instructions->push_back(move(generateExpression(returnStatement->expression, instructions), reg(Register::AX)));
		instructions->push_back(std::make_shared<ReturnInstruction>());
	}

	void CodeGenerator::generateIfStatement(StatementPtr statement, InstructionList* instructions) {
		std::shared_ptr<IfStatement> ifStatement = static_pointer_cast<IfStatement>(statement);

		uint64_t index = generateIfIndex();
		std::string falseLabel = ".IF" + std::to_string(index);
		std::string trueLabel = ".IT" + std::to_string(index);
		std::string endLabel = ".IE" + std::to_string(index);

		if (!ifStatement->elseBody) falseLabel = endLabel;

		OperandPtr temp = generateExpression(ifStatement->condition, instructions);
		instructions->push_back(cmp(temp, intConst(0)));
		instructions->push_back(jmp(falseLabel, ComparisonType::Equal));
		generateStatement(ifStatement->ifBody, instructions);

		if (ifStatement->elseBody) {
			instructions->push_back(jmp(endLabel, ComparisonType::None));
			instructions->push_back(label(falseLabel));
			generateStatement(ifStatement->elseBody, instructions);
		}
		instructions->push_back(label(endLabel));
	}

	void CodeGenerator::generateLoopStatement(StatementPtr statement, InstructionList* instructions) {
		std::shared_ptr<LoopStatement> loopStatement = static_pointer_cast<LoopStatement>(statement);

		uint64_t index = generateIfIndex();
		std::string startLabel = ".LS" + std::to_string(index);
		std::string exitLabel = ".LE" + std::to_string(index);

		m_loopStack.push_back({ startLabel, exitLabel });

		instructions->push_back(label(startLabel));
		generateStatement(loopStatement->loopBody, instructions);
		instructions->push_back(jmp(startLabel, ComparisonType::None));
		instructions->push_back(label(exitLabel));

		m_loopStack.pop_back();
	}

	void CodeGenerator::generateWhileStatement(StatementPtr statement, InstructionList* instructions) {
		std::shared_ptr<WhileStatement> whileStatement = static_pointer_cast<WhileStatement>(statement);

		uint64_t index = generateIfIndex();
		std::string startLabel = ".LS" + std::to_string(index);
		std::string exitLabel = ".LE" + std::to_string(index);

		m_loopStack.push_back({ startLabel, exitLabel });

		instructions->push_back(label(startLabel));
		OperandPtr temp = generateExpression(whileStatement->condition, instructions);
		instructions->push_back(cmp(temp, intConst(0)));
		instructions->push_back(jmp(exitLabel, ComparisonType::Equal));
		generateStatement(whileStatement->loopBody, instructions);
		instructions->push_back(jmp(startLabel, ComparisonType::None));
		instructions->push_back(label(exitLabel));

		m_loopStack.pop_back();
	}

	void CodeGenerator::generateBreakStatement(StatementPtr statement, InstructionList* instructions) {
		std::shared_ptr<BreakStatement> breakStatement = static_pointer_cast<BreakStatement>(statement);

		instructions->push_back(jmp(m_loopStack[m_loopStack.size() - 1].exitLabel, ComparisonType::None));
	}

	void CodeGenerator::generateContinueStatement(StatementPtr statement, InstructionList* instructions) {
		std::shared_ptr<BreakStatement> breakStatement = static_pointer_cast<BreakStatement>(statement);

		instructions->push_back(jmp(m_loopStack[m_loopStack.size() - 1].startLabel, ComparisonType::None));
	}

	OperandPtr CodeGenerator::generateExpression(ExpressionPtr expression, InstructionList* instructions) {
		switch (expression->getType()) {
			case NodeType::IntegerLiteral: return generateIntConstant(expression, instructions);
			case NodeType::UnaryOperation: return generateUnaryOperation(expression, instructions);
			case NodeType::BinaryOperation: return generateBinaryOperation(expression, instructions);
			case NodeType::Assignment: return generateAssignment(expression, instructions);
			case NodeType::Identifier: return getIdentifier(expression);
			case NodeType::Call: return generateCall(expression, instructions);
			case NodeType::Cast: return generateCast(expression, instructions);
			default: return nullptr;
		}
	}

	OperandPtr CodeGenerator::getIdentifier(ExpressionPtr expression) {
		std::shared_ptr<Identifier> id = static_pointer_cast<Identifier>(expression);
		return m_localVarMap[id->idToken.value];
	}

	OperandPtr CodeGenerator::generateUnaryOperation(ExpressionPtr expression, InstructionList* instructions) {
		std::shared_ptr<UnaryOperation> unaryOp = static_pointer_cast<UnaryOperation>(expression);
		std::shared_ptr<TempOperand> destination = std::make_shared<TempOperand>(allocateStack(Size::DWord), Size::DWord);

		instructions->push_back(move(generateExpression(unaryOp->expression, instructions), destination));

		switch (unaryOp->operatorToken.type) {
		case MINUS:
			instructions->push_back(negate(destination));
			break;
		case TILDE:
			instructions->push_back(bitwiseNot(destination));
			break;
		}

		return destination;
	}

	OperandPtr CodeGenerator::generateIntConstant(ExpressionPtr expression, InstructionList* instructions) {
		std::shared_ptr<IntegerLiteral> intLiteral = static_pointer_cast<IntegerLiteral>(expression);

		return intConst(intLiteral->value);
	}

	OperandPtr CodeGenerator::generateBinaryOperation(ExpressionPtr expression, InstructionList* instructions) {
		std::shared_ptr<BinaryOperation> binaryOp = static_pointer_cast<BinaryOperation>(expression);

		switch (binaryOp->operatorToken.type) {
			case PLUS:
			case MINUS:
			case STAR:
				return generateArithmeticOperation(binaryOp, instructions);
			case SLASH:
				return idiv(binaryOp, instructions);
			case LESS:
			case LESS_EQUAL:
			case GREATER:
			case GREATER_EQUAL:
			case EQUAL_EQUAL:
			case BANG_EQUAL:
				return generateRelational(binaryOp, instructions);
			case OR:
			case AND:
				return generateLogical(binaryOp, instructions);
		}

	}
	
	OperandPtr CodeGenerator::generateArithmeticOperation(std::shared_ptr<BinaryOperation> expression, InstructionList* instructions) {
		std::shared_ptr<TempOperand> destination = std::make_shared<TempOperand>(allocateStack(Size::DWord), Size::DWord);
		instructions->push_back(move(generateExpression(expression->left, instructions), destination));

		switch (expression->operatorToken.type) {
			case PLUS:
				instructions->push_back(add(destination, generateExpression(expression->right, instructions)));
				break;
			case MINUS:
				instructions->push_back(sub(destination, generateExpression(expression->right, instructions)));
				break;
			case STAR:
				instructions->push_back(mul(destination, generateExpression(expression->right, instructions)));
				break;
		}

		return destination;
	}

	ComparisonType getComparisonType(TokenType tokType) {
		switch(tokType) {
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
				break;
		}
	}

	OperandPtr CodeGenerator::generateRelational(std::shared_ptr<BinaryOperation> expression, InstructionList* instructions) {
		std::shared_ptr<TempOperand> destination = std::make_shared<TempOperand>(allocateStack(Size::DWord), Size::Byte);
		ComparisonType type = getComparisonType(expression->operatorToken.type);
		instructions->push_back(cmp(generateExpression(expression->left, instructions)
			, generateExpression(expression->right, instructions)));
		instructions->push_back(set(destination, type));
		return destination;
	}

	OperandPtr CodeGenerator::generateLogical(std::shared_ptr<BinaryOperation> expression, InstructionList* instructions) {
		std::shared_ptr<TempOperand> destination = std::make_shared<TempOperand>(allocateStack(Size::DWord), Size::DWord);

		uint64_t index = generateComparisonIndex();
		std::string falseLabel = ".CF" + std::to_string(index);
		std::string trueLabel = ".CT" + std::to_string(index);
		std::string endLabel = ".CE" + std::to_string(index);

		if (expression->operatorToken.type == AND) {
			OperandPtr a = generateExpression(expression->left, instructions);
			instructions->push_back(cmp(a, intConst(0)));
			instructions->push_back(jmp(falseLabel, ComparisonType::Equal));
			OperandPtr b = generateExpression(expression->right, instructions);
			instructions->push_back(cmp(b, intConst(0)));
			instructions->push_back(jmp(falseLabel, ComparisonType::Equal));

			// Only happens if true
			instructions->push_back(move(intConst(1), destination));
			instructions->push_back(jmp(endLabel, ComparisonType::None));

			// False label
			instructions->push_back(label(falseLabel));
			instructions->push_back(move(intConst(0), destination));
		}
		else {
			OperandPtr a = generateExpression(expression->left, instructions);
			instructions->push_back(cmp(a, intConst(1)));
			instructions->push_back(jmp(trueLabel, ComparisonType::Equal));
			OperandPtr b = generateExpression(expression->right, instructions);
			instructions->push_back(cmp(b, intConst(1)));
			instructions->push_back(jmp(trueLabel, ComparisonType::Equal));

			// Only happens if false
			instructions->push_back(move(intConst(0), destination));
			instructions->push_back(jmp(endLabel, ComparisonType::None));

			// True label
			instructions->push_back(label(trueLabel));
			instructions->push_back(move(intConst(1), destination));
		}

		// End
		instructions->push_back(label(endLabel));

		return destination;
	}

	OperandPtr CodeGenerator::generateAssignment(ExpressionPtr expression, InstructionList* instructions) {
		std::shared_ptr<Assignment> assignment = static_pointer_cast<Assignment>(expression);
		OperandPtr temp = generateExpression(assignment->right, instructions);
		OperandPtr var = generateExpression(assignment->left, instructions);
		instructions->push_back(move(temp, var));
		return var;
	}

	OperandPtr CodeGenerator::generateCall(ExpressionPtr expression, InstructionList* instructions) {
		std::shared_ptr<Call> callExpr = static_pointer_cast<Call>(expression);
		if(m_platform == Platform::Mac) callExpr->funcToken.value = "_" + callExpr->funcToken.value;
		int stackPadding = (m_platform == Platform::Win ? 32 : 0);
		if (callExpr->argumentList.size() % 2 == 0) stackPadding += 8;

		std::vector<Register> argumentRegisters = (m_platform == Platform::Win) 
			? std::vector<Register>{ Register::CX, Register::DX, Register::R8, Register::R9 }
			: std::vector<Register>{ Register::DI, Register::SI, Register::DX, Register::CX, Register::R8, Register::R9 };

		instructions->push_back(sub(reg(Register::SP, Size::QWord), intConst(stackPadding)));
		bool addToStack = false;
		size_t m = (callExpr->argumentList.size() > argumentRegisters.size() ? addToStack = true, argumentRegisters.size() : callExpr->argumentList.size());
		for (size_t i = 0; i < m; i++)
			instructions->push_back(move(generateExpression(callExpr->argumentList[i]->expression, instructions), reg(argumentRegisters[i], Size::DWord)));

		uint32_t stackArgs = 0;

		if (addToStack) {
			for (size_t i = callExpr->argumentList.size() - 1; i >= argumentRegisters.size(); i--) {
				instructions->push_back(push(generateExpression(callExpr->argumentList[i]->expression, instructions)));
				stackArgs++;
			}
		}
		instructions->push_back(call(callExpr->funcToken.value));
		instructions->push_back(add(reg(Register::SP, Size::QWord), intConst(8 * stackArgs + stackPadding)));
		
		return reg(Register::AX, Size::DWord);
	}

	OperandPtr CodeGenerator::generateCast(ExpressionPtr expression, InstructionList* instructions) {
		std::shared_ptr<CastExpression> cast = static_pointer_cast<CastExpression>(expression);
		Size typeSize = getTypeSize(cast->target);
		OperandPtr temp = std::make_shared<TempOperand>(allocateStack(typeSize), typeSize);
		OperandPtr exprOutput = generateExpression(cast->expr, instructions);
		if((int)getTypeSize(cast->expr->expressionType) < (int)typeSize) {
			instructions->push_back(move(exprOutput, temp));
		}
		else {
			std::cout << "SIZE1: " << (int)exprOutput->getSize() << " ";
			exprOutput->setSize(getTypeSize(cast->target));
			std::cout << "SIZE2: " << (int)exprOutput->getSize() << " ";
			instructions->push_back(move(exprOutput, temp));
		}
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

	InstructionPtr CodeGenerator::move(OperandPtr source, OperandPtr destination) {
		return std::make_shared<MoveInstruction>(source, destination);
	}

	InstructionPtr CodeGenerator::negate(OperandPtr source) {
		return std::make_shared<NegateInstruction>(source);
	}

	InstructionPtr CodeGenerator::bitwiseNot(OperandPtr source) {
		return std::make_shared<BitwiseNotInstruction>(source);
	}

	InstructionPtr CodeGenerator::add(OperandPtr destination, OperandPtr other) {
		return std::make_shared<AddInstruction>(destination, other);
	}

	InstructionPtr CodeGenerator::sub(OperandPtr destination, OperandPtr other) {
		return std::make_shared<SubtractInstruction>(destination, other);
	}

	InstructionPtr CodeGenerator::mul(OperandPtr destination, OperandPtr other) {
		return std::make_shared<MultiplyInstruction>(destination, other);
	}

	OperandPtr CodeGenerator::idiv(std::shared_ptr<BinaryOperation> division, InstructionList* instructions) {
		OperandPtr right = generateExpression(division->right, instructions);
		std::shared_ptr<TempOperand> temp = std::make_shared<TempOperand>(allocateStack(Size::DWord), Size::DWord);
		instructions->push_back(move(right, temp));
		OperandPtr left = generateExpression(division->left, instructions);
		instructions->push_back(move(left, reg(Register::AX)));
		instructions->push_back(std::make_shared<CdqInstruction>());
		instructions->push_back(std::make_shared<DivInstruction>(temp));
		return reg(Register::AX);
	}

	OperandPtr CodeGenerator::reg(Register register_, Size size) {
		return std::make_shared<RegisterOperand>(register_, size);
	}

	InstructionPtr CodeGenerator::cmp(OperandPtr left, OperandPtr right) {
		return std::make_shared<CompareInstruction>(left, right);
	}

	InstructionPtr CodeGenerator::set(OperandPtr operand, ComparisonType type) {
		return std::make_shared<SetInstruction>(operand, type);
	}

	InstructionPtr CodeGenerator::jmp(const std::string& label, ComparisonType type) {
		return std::make_shared<JumpInstruction>(label, type);
	}

	InstructionPtr CodeGenerator::call(const std::string& func) {
		return std::make_shared<CallInstruction>(func);
	}

	InstructionPtr CodeGenerator::push(OperandPtr src) {
		return std::make_shared<PushInstruction>(src);
	}

	OperandPtr CodeGenerator::intConst(int64_t integer) {
		return std::make_shared<IntegerConstant>(integer);
	}

	int64_t CodeGenerator::allocateStack(Size size) {
		m_currentStackIndex += (int)size;
		return m_currentStackIndex;
	}

	void popStack(uint64_t ammount);

	void CodeGenerator::validateInstructions(InstructionList* instructions) {
		for(size_t i = 0; i < instructions->size(); i++) {
			switch ((*instructions)[i]->getType()) {
			case InstructionType::FunctionDefinition: validateFunction((*instructions)[i]); break;
			case InstructionType::Move: validateMove(instructions, i); break;
			case InstructionType::Add: validateAdd(instructions, i); break;
			case InstructionType::Subtract: validateSub(instructions, i); break;
			case InstructionType::Multiply: validateMul(instructions, i); break;
			case InstructionType::Divide: validateDiv(instructions, i); break;
			case InstructionType::Compare: validateCmp(instructions, i); break;
			case InstructionType::Push: validatePush(instructions, i); break;
			}
		}
	}

	void CodeGenerator::validateFunction(InstructionPtr instruction) {
		std::shared_ptr<FunctionDefInstruction> func = static_pointer_cast<FunctionDefInstruction>(instruction);
		validateInstructions(&func->instructions);
	}

	bool isTemp(OperandPtr operand) {
		return operand->getType() == OperandType::Temp;
	}

	void CodeGenerator::validateMoveOperands(InstructionList* instructions, size_t i, OperandPtr source, OperandPtr* destination) {
		if (isTemp(source) && isTemp(*destination)) {
			OperandPtr scratchReg = reg(Register::R10, source->getSize());
			OperandPtr oldDestination = *destination;
			*destination = scratchReg;
			instructions->emplace(instructions->begin() + i + 1, move(scratchReg, oldDestination));
		}
	}

	void CodeGenerator::validateBinOperands(InstructionList* instructions, size_t i, OperandPtr source, OperandPtr* other) {
		if (isTemp(source) && isTemp(*other)) {
			OperandPtr scratchReg = reg(Register::R10);
			OperandPtr oldOther = *other;
			*other = scratchReg;
			instructions->emplace(instructions->begin() + i, move(oldOther, scratchReg));
		}
	}

	void CodeGenerator::validateMove(InstructionList* instructions, size_t i) {
		std::shared_ptr<MoveInstruction> moveInstruction = static_pointer_cast<MoveInstruction>((*instructions)[i]);
		if(moveInstruction->destination->getSize() > moveInstruction->source->getSize()) {
			OperandPtr scratchReg = reg(Register::AX, moveInstruction->destination->getSize());
			OperandPtr oldDest = moveInstruction->destination;
			moveInstruction->destination = scratchReg;
			moveInstruction->signExtend = true;
			instructions->emplace(instructions->begin() + i + 1, move(scratchReg, oldDest));
		}
		else
			validateMoveOperands(instructions, i, moveInstruction->source, &moveInstruction->destination);
	}

	void CodeGenerator::validateAdd(InstructionList* instructions, size_t i) {
		std::shared_ptr<AddInstruction> addInstruction = static_pointer_cast<AddInstruction>((*instructions)[i]);
		validateBinOperands(instructions, i, addInstruction->destination, &addInstruction->other);
	}

	void CodeGenerator::validateSub(InstructionList* instructions, size_t i) {
		std::shared_ptr<SubtractInstruction> subInstruction = static_pointer_cast<SubtractInstruction>((*instructions)[i]);
		validateBinOperands(instructions, i, subInstruction->destination, &subInstruction->other);
	}

	void CodeGenerator::validateMul(InstructionList* instructions, size_t i) {
		std::shared_ptr<MultiplyInstruction> mulInstruction = static_pointer_cast<MultiplyInstruction>((*instructions)[i]);
		if (isTemp(mulInstruction->destination)) {
			OperandPtr scratchReg = reg(Register::R11);
			OperandPtr oldDestination = mulInstruction->destination;
			mulInstruction->destination = scratchReg;
			instructions->emplace(instructions->begin() + i, move(oldDestination, scratchReg));
			instructions->emplace(instructions->begin() + i + 2, move(scratchReg, oldDestination));
		}
	}

	void CodeGenerator::validateDiv(InstructionList* instructions, size_t i) {

	}

	void CodeGenerator::validateCmp(InstructionList* instructions, size_t i) {
		std::shared_ptr<CompareInstruction> cmpInstruction = static_pointer_cast<CompareInstruction>((*instructions)[i]);
		if (cmpInstruction->left->getType() == OperandType::IntegerConstant
			|| isTemp(cmpInstruction->left)) {
			OperandPtr scratchReg = reg(Register::AX);
			OperandPtr oldLeft = cmpInstruction->left;
			cmpInstruction->left = scratchReg;
			instructions->emplace(instructions->begin() + i, move(oldLeft, scratchReg));
		}
	}

	void CodeGenerator::validatePush(InstructionList* instructions, size_t i) {
		std::shared_ptr<PushInstruction> pushInstruction = static_pointer_cast<PushInstruction>((*instructions)[i]);
		if (pushInstruction->src->getType() != OperandType::IntegerConstant
			&& pushInstruction->src->getSize() != Size::QWord) {
			OperandPtr scratchReg = reg(Register::AX, Size::QWord);
			instructions->emplace(instructions->begin() + i, move(pushInstruction->src, scratchReg));
			pushInstruction->src = scratchReg;
		}
	}
}