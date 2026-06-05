// IntelCodeEmission.cpp
// Contains the IntelCodeEmmision method implementations
// Copyright (c) 2025-present, Stylianos Kementzetzidis

#include "IntelCodeEmission.h"

namespace Fractal {
    // Renders a byte string as a NASM `db` operand list: printable ASCII goes
    // into quoted chunks, everything else (newlines, quotes, NULs, high bytes)
    // becomes a comma-separated decimal byte. Example:
    //     "Hello\nWorld!\0"  ->  "Hello", 10, "World!", 0
    // NASM tolerates back-to-back chunks separated by commas, and any byte not
    // legal inside quotes (a quote itself, control chars, the null terminator)
    // must be numeric. Always emits at least one byte so callers don't have
    // to special-case the empty string.
    static std::string nasmDbLiteral(const std::string& bytes) {
        std::string out;
        bool inQuotes = false;
        auto closeQuotes = [&]() {
            if (inQuotes) { out += '"'; inQuotes = false; }
        };
        auto separator = [&]() {
            if (!out.empty()) out += ", ";
        };
        for (unsigned char c : bytes) {
            bool printable = c >= 0x20 && c <= 0x7e && c != '"';
            if (printable) {
                if (!inQuotes) {
                    separator();
                    out += '"';
                    inQuotes = true;
                }
                out += static_cast<char>(c);
            } else {
                closeQuotes();
                separator();
                out += std::to_string(static_cast<unsigned>(c));
            }
        }
        closeQuotes();
        return out.empty() ? "0" : out;
    }

    // Single source of truth for platform-specific name mangling. Mach-O (Mac)
    // expects leading underscores on every C symbol; PE/ELF (Win/Linux) does not.
    std::string IntelCodeEmission::mangle(const std::string& name) const {
        return m_platform == Platform::Mac ? "_" + name : name;
    }

    const std::string& IntelCodeEmission::emit(const InstructionList* instructions,
                                               const CodeGenObjects& codeGenObjects,
                                               Platform platform) {
        m_platform = platform;
        m_instructions = instructions;
        m_externals = codeGenObjects.externals;
        m_stringMap = codeGenObjects.strings;

        if (!m_externals->empty()) {
            std::string writeExt = "extern ";
            for (const auto& str : *m_externals)
                writeExt += mangle(str) + ", ";

            writeLine(writeExt);
        }

        writeLine("section .text");

        for (const auto& instruction : *instructions)
            emitInstruction(instruction.get());

        writeLine("section .data");

        // m_stringMap is keyed by the string's literal contents; the value
        // is the unique label the codegen assigned. Each entry becomes:
        //     <label>: db <nasm-escaped bytes>, 0
        // The trailing NUL is appended so C-style consumers (printf, etc.)
        // can treat the address as a normal null-terminated string.
        for (const auto& [content, label] : *m_stringMap) {
            std::string payload = content;
            payload.push_back('\0');
            writeLine(label + ": db " + nasmDbLiteral(payload));
        }

        return m_output;
    }

    const std::string& IntelCodeEmission::output() const {
        return m_output;
    }

    std::string IntelCodeEmission::getSize(Size size) {
        switch (size) {
        case Size::Byte:
            return "BYTE";
        case Size::Word:
            return "WORD";
        case Size::DWord:
            return "DWORD";
        case Size::QWord:
            return "QWORD";
        default:
            return "";
        }
    }

    void IntelCodeEmission::emitInstruction(const Instruction* instruction) {
        switch (instruction->getType()) {
        case InstructionType::FunctionDefinition:
            emitFunctionDefinition(instruction);
            return;
        case InstructionType::Move:
            emitMove(instruction);
            return;
        case InstructionType::Lea:
            emitLea(instruction);
            return;
        case InstructionType::LeaLabel:
            emitLeaLabel(instruction);
            return;
        case InstructionType::Negate:
            emitNegation(instruction);
            return;
        case InstructionType::BitwiseNot:
            emitBitwiseNot(instruction);
            return;
        case InstructionType::Add:
            emitAdd(instruction);
            return;
        case InstructionType::Subtract:
            emitSub(instruction);
            return;
        case InstructionType::Multiply:
            emitMul(instruction);
            return;
        case InstructionType::Cdq:
            emitCdq();
            return;
        case InstructionType::Divide:
            emitIdiv(instruction);
            return;
        case InstructionType::Compare:
            emitCmp(instruction);
            return;
        case InstructionType::Set:
            emitSet(instruction);
            return;
        case InstructionType::Jump:
            emitJmp(instruction);
            return;
        case InstructionType::Label:
            emitLabel(instruction);
            return;
        case InstructionType::Call:
            emitCall(instruction);
            return;
        case InstructionType::Return:
            emitFunctionEpilogue();
            emitReturn();
            return;
        case InstructionType::Push:
            emitPush(instruction);
            return;
        default:
            return;
        }
    }

    void IntelCodeEmission::emitFunctionDefinition(const Instruction* instruction) {
        auto* functionDefinition = static_cast<const FunctionDefInstruction*>(instruction);

        const std::string sym = mangle(functionDefinition->name);
        writeLine("global " + sym);
        label(sym);

        emitFunctionPrologue(functionDefinition->stackAlloc);

        for (const auto& instruction : functionDefinition->instructions)
            emitInstruction(instruction.get());
    }

    void IntelCodeEmission::emitFunctionPrologue(uint64_t stackAlloc) {
        writeILine("push rbp");
        writeILine("mov rbp, rsp");
        writeILine("sub rsp, " + std::to_string(stackAlloc));
    }

    void IntelCodeEmission::emitFunctionEpilogue() {
        writeILine("mov rsp, rbp");
        writeILine("pop rbp");
    }

    void IntelCodeEmission::emitMove(const Instruction* instruction) {
        auto* moveInstruction = static_cast<const MoveInstruction*>(instruction);

        writeILine((moveInstruction->signExtend ? "movsx " : "mov ") +
                   getOperandStr(moveInstruction->destination, moveInstruction->destSize) + ", " +
                   getOperandStr(moveInstruction->source, moveInstruction->srcSize));
    }

    void IntelCodeEmission::emitLea(const Instruction* instruction) {
        auto* leaInstruction = static_cast<const LeaInstruction*>(instruction);

        // `lea reg, [mem]` — register form is QWord on x86_64 (we're loading
        // a 64-bit pointer). getOperandStr renders the memory operand with
        // its own size prefix; lea ignores it but the assembler tolerates it.
        writeILine("lea " + getOperandStr(leaInstruction->destination, Size::QWord) + ", " +
                   getOperandStr(leaInstruction->source));
    }

    void IntelCodeEmission::emitLeaLabel(const Instruction* instruction) {
        auto* leaLabelInstruction = static_cast<const LeaLabelInstruction*>(instruction);

        writeILine("lea " + getOperandStr(leaLabelInstruction->destination, Size::QWord) + ", " +
                   "[rel " + leaLabelInstruction->label + "]");
    }

    void IntelCodeEmission::emitNegation(const Instruction* instruction) {
        auto* negInstruction = static_cast<const NegateInstruction*>(instruction);

        writeILine("neg " + getOperandStr(negInstruction->source));
    }

    void IntelCodeEmission::emitBitwiseNot(const Instruction* instruction) {
        auto* notInstruction = static_cast<const BitwiseNotInstruction*>(instruction);

        writeILine("not " + getOperandStr(notInstruction->source));
    }

    void IntelCodeEmission::emitAdd(const Instruction* instruction) {
        auto* addInstruction = static_cast<const AddInstruction*>(instruction);

        writeILine("add " + getOperandStr(addInstruction->destination) + ", " +
                   getOperandStr(addInstruction->other));
    }

    void IntelCodeEmission::emitSub(const Instruction* instruction) {
        auto* subInstruction = static_cast<const SubtractInstruction*>(instruction);

        writeILine("sub " + getOperandStr(subInstruction->destination) + ", " +
                   getOperandStr(subInstruction->other));
    }

    void IntelCodeEmission::emitMul(const Instruction* instruction) {
        auto* mulInstruction = static_cast<const MultiplyInstruction*>(instruction);

        writeILine("imul " + getOperandStr(mulInstruction->destination) + ", " +
                   getOperandStr(mulInstruction->other));
    }

    void IntelCodeEmission::emitCdq() {
        writeILine("cdq");
    }

    void IntelCodeEmission::emitIdiv(const Instruction* instruction) {
        auto* divInstruction = static_cast<const DivInstruction*>(instruction);

        writeILine("idiv " + getOperandStr(divInstruction->destination));
    }

    void IntelCodeEmission::emitCmp(const Instruction* instruction) {
        auto* cmpInstruction = static_cast<const CompareInstruction*>(instruction);

        writeILine("cmp " + getOperandStr(cmpInstruction->left) + ", " +
                   getOperandStr(cmpInstruction->right));
    }

    void IntelCodeEmission::emitSet(const Instruction* instruction) {
        auto* setInstruction = static_cast<const SetInstruction*>(instruction);

        writeILine("set" + getComparisonType(setInstruction->type) + " " +
                   getOperandStr(setInstruction->destination));
    }

    void IntelCodeEmission::emitJmp(const Instruction* instruction) {
        auto* jmpInstruction = static_cast<const JumpInstruction*>(instruction);

        writeILine("j" + getComparisonType(jmpInstruction->type) + " " + jmpInstruction->label);
    }

    void IntelCodeEmission::emitLabel(const Instruction* instruction) {
        auto* label = static_cast<const Label*>(instruction);

        writeLine(label->name + ":");
    }

    void IntelCodeEmission::emitCall(const Instruction* instruction) {
        auto* call = static_cast<const CallInstruction*>(instruction);
        writeILine("call " + mangle(call->func));
    }

    void IntelCodeEmission::emitPush(const Instruction* instruction) {
        auto* push = static_cast<const PushInstruction*>(instruction);

        writeILine("push " + getOperandStr(push->src));
    }

    void IntelCodeEmission::emitReturn() {
        writeILine("ret");
    }

    std::string IntelCodeEmission::getRegister(OperandPtr operand, Size externalSize) {
        auto* reg = static_cast<const RegisterOperand*>(operand.get());
        Size s = ((bool)externalSize ? externalSize : reg->size);
        std::string toRet = "";
        if ((uint8_t)reg->reg < 8) {
            switch (s) {
            case Size::QWord:
                toRet = "r";
                break;
            case Size::DWord:
                toRet = "e";
                break;
            }
        }
        switch (reg->reg) {
        case Register::AX:
            toRet += "ax";
            break;
        case Register::BX:
            toRet += "bx";
            break;
        case Register::CX:
            toRet += "cx";
            break;
        case Register::DX:
            toRet += "dx";
            break;
        case Register::DI:
            toRet += "di";
            break;
        case Register::SI:
            toRet += "si";
            break;
        case Register::BP:
            toRet += "bp";
            break;
        case Register::SP:
            toRet += "sp";
            break;
        case Register::R8:
            toRet += "r8";
            break;
        case Register::R9:
            toRet += "r9";
            break;
        case Register::R10:
            toRet += "r10";
            break;
        case Register::R11:
            toRet += "r11";
            break;
        default:
            return "";
        }
        if ((uint8_t)reg->reg >= 8) {
            switch (s) {
            case Size::QWord:
                break;
            case Size::DWord:
                toRet += "d";
                break;
            }
        }
        return toRet;
    }

    std::string IntelCodeEmission::getOperandStr(OperandPtr operand, Size externalSize) {
        if (!operand)
            return "";
        switch (operand->getType()) {
        case OperandType::IntegerConstant:
            return std::to_string(static_cast<const IntegerConstant*>(operand.get())->integer);
        case OperandType::Register:
            return getRegister(operand, externalSize);
        case OperandType::Temp:
            return getTemp(operand, externalSize);
        case OperandType::Indirect:
            return getIndirect(operand, externalSize);
        default:
            return "";
        }
    }

    std::string getSizeMemory(Size size) {
        switch (size) {
        case Size::Byte:
            return "BYTE";
        case Size::Word:
            return "WORD";
        case Size::DWord:
            return "DWORD";
        case Size::QWord:
            return "QWORD";
        default:
            return "";
        }
    }

    std::string IntelCodeEmission::getTemp(OperandPtr operand, Size externalSize) {
        auto* temp = static_cast<const TempOperand*>(operand.get());
        std::string sign = temp->stackOffest < 0 ? "+" : "-";
        return getSizeMemory((bool)externalSize ? externalSize : temp->size) + " [rbp " + sign +
               " " + std::to_string(abs(temp->stackOffest)) + "]";
    }

    // Render `[reg]` with the appropriate size prefix. The base register is
    // always emitted at QWord width since pointers are 64-bit; the size
    // prefix on the memory operand reflects how many bytes are read/written.
    std::string IntelCodeEmission::getIndirect(OperandPtr operand, Size externalSize) {
        auto* ind = static_cast<const IndirectOperand*>(operand.get());
        Size sz = (bool)externalSize ? externalSize : ind->size;
        auto baseReg = std::make_shared<RegisterOperand>(ind->baseReg, Size::QWord);
        return getSizeMemory(sz) + " [" + getRegister(baseReg, Size::QWord) + "]";
    }

    std::string IntelCodeEmission::getComparisonType(ComparisonType type) {
        switch (type) {
        case ComparisonType::Equal:
            return "e";
        case ComparisonType::NotEqual:
            return "ne";
        case ComparisonType::Greater:
            return "g";
        case ComparisonType::GreaterEqual:
            return "ge";
        case ComparisonType::Less:
            return "l";
        case ComparisonType::LessEqual:
            return "le";
        case ComparisonType::None:
            return "mp";
        }
        return "";
    }

    void IntelCodeEmission::write(const std::string& text) {
        m_output += text;
    }

    void IntelCodeEmission::writeLine(const std::string& line) {
        write(line + "\n");
    }

    void IntelCodeEmission::writeILine(const std::string& line) {
        writeLine("    " + line);
    }

    void IntelCodeEmission::label(const std::string& name) {
        writeLine(name + ":");
    }
} // namespace Fractal