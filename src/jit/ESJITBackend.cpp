#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITBackend.h"

#include "ESJIT.h"
#include "ESIR.h"
#include "ESIRType.h"
#include "ESGraph.h"
#include "bytecode/ByteCode.h"
#include "runtime/Operations.h"
#include "runtime/ExecutionContext.h"
#include "vm/ESVMInstance.h"

#include "nanojit.h"

namespace escargot {
namespace ESJIT {

using namespace nanojit;

#ifdef DEBUG
#define DEBUG_ONLY_NAME(name)   ,#name
#else
#define DEBUG_ONLY_NAME(name)
#endif

#define CI(name, args) \
    {(uintptr_t) (&name), args, nanojit::ABI_CDECL, /*isPure*/0, ACCSET_STORE_ANY \
     DEBUG_ONLY_NAME(name)}

CallInfo plusOpCallInfo = CI(plusOp, CallInfo::typeSig2(ARGTYPE_D, ARGTYPE_D, ARGTYPE_D));
CallInfo minusOpCallInfo = CI(minusOp, CallInfo::typeSig2(ARGTYPE_D, ARGTYPE_D, ARGTYPE_D));
CallInfo contextResolveBindingCallInfo = CI(contextResolveBinding, CallInfo::typeSig3(ARGTYPE_P, ARGTYPE_P, ARGTYPE_P, ARGTYPE_P));
CallInfo objectDefinePropertyOrThrowCallInfo = CI(objectDefinePropertyOrThrow, CallInfo::typeSig3(ARGTYPE_V, ARGTYPE_P, ARGTYPE_D, /*ARGTYPE_B, ARGTYPE_B, ARGTYPE_B,*/ ARGTYPE_D));
CallInfo esFunctionObjectCallCallInfo = CI(esFunctionObjectCall, CallInfo::typeSig6(ARGTYPE_D, ARGTYPE_P, ARGTYPE_D, ARGTYPE_D, ARGTYPE_P, ARGTYPE_I, ARGTYPE_B));
CallInfo ESObjectSetOpCallInfo = CI(ESObjectSetOp, CallInfo::typeSig3(ARGTYPE_D, ARGTYPE_D, ARGTYPE_D, ARGTYPE_D));
#if 0
CallInfo resolveNonDataPropertyInfo = CI(resolveNonDataProperty, CallInfo::typeSig2(ARGTYPE_D, ARGTYPE_P, ARGTYPE_P));
#else
CallInfo resolveNonDataPropertyInfo = CI(resolveNonDataProperty, CallInfo::typeSig2(ARGTYPE_D, ARGTYPE_P, ARGTYPE_Q));
#endif
#ifndef NDEBUG
CallInfo logIntCallInfo = CI(jitLogIntOperation, CallInfo::typeSig2(ARGTYPE_V, ARGTYPE_I, ARGTYPE_P));
CallInfo logDoubleCallInfo = CI(jitLogDoubleOperation, CallInfo::typeSig2(ARGTYPE_V, ARGTYPE_D, ARGTYPE_P));
CallInfo logPointerCallInfo = CI(jitLogPointerOperation, CallInfo::typeSig2(ARGTYPE_V, ARGTYPE_P, ARGTYPE_P));
#define JIT_LOG_I(arg, msg) { LIns* args[] = { m_out.insImmP(msg), arg }; m_out.insCall(&logIntCallInfo, args); }
#define JIT_LOG_D(arg, msg) { LIns* args[] = { m_out.insImmP(msg), arg}; m_out.insCall(&logDoubleCallInfo, args); }
#define JIT_LOG_P(arg, msg) { LIns* args[] = { m_out.insImmP(msg), arg}; m_out.insCall(&logPointerCallInfo, args); }
#define JIT_LOG_S(arg, msg) { LIns* args[] = { m_out.insImmP(msg), arg}; m_out.insCall(&logStringCallInfo, args); }
#define JIT_LOG(arg, msg) { \
    if (arg->isI()) JIT_LOG_I(arg, msg); \
    if (arg->isD()) JIT_LOG_D(arg, msg); \
    if (arg->isP()) JIT_LOG_P(arg, msg); \
}
#else
#define JIT_LOG_I(arg, msg)
#define JIT_LOG_D(arg, msg)
#define JIT_LOG_P(arg, msg)
#define JIT_LOG_S(arg, msg)
#define JIT_LOG(arg, msg)
#endif

NativeGenerator::NativeGenerator(ESGraph* graph)
    : m_graph(graph),
    m_tmpToLInsMapping(graph->tempRegisterSize()),
    m_varToLInsMapping(10), // TODO
    m_stackPtr(nullptr),
    m_instance(nullptr),
    m_context(nullptr),
    m_alloc(new Allocator()),
    m_codeAlloc(new CodeAlloc(&m_config)),
    m_assm(new Assembler(*m_codeAlloc, *m_alloc, *m_alloc, &m_lc, m_config)),
    m_buf(new LirBuffer(*m_alloc)),
    m_f(new Fragment(NULL verbose_only(, 0))),
    m_out(m_buf, m_config)
{
#ifdef DEBUG
    if (ESVMInstance::currentInstance()->m_verboseJIT)
        m_lc.lcbits = LC_ReadLIR | LC_Native;
    else
        m_lc.lcbits = 0;
    m_buf->printer = new LInsPrinter(*m_alloc, 1);
#else
    m_lc.lcbits = 0;
#endif

    m_f->lirbuf = m_buf;
}

NativeGenerator::~NativeGenerator()
{
    // TODO : separate long-lived and short-lived data structures
#ifdef DEBUG
    delete m_buf->printer;
#endif
    delete m_alloc;
    //delete m_codeAlloc;
    delete m_assm;
    delete m_buf;
    delete m_f;
}

LIns* NativeGenerator::generateOSRExit(size_t currentByteCodeIndex)
{
    m_out.insStore(LIR_sti, m_oneI, m_context, ExecutionContext::offsetofInOSRExit(), 1);
    LIns* bytecode = m_out.insImmI(currentByteCodeIndex);
    LIns* boxedIndex = boxESValue(bytecode, TypeInt32);
    return m_out.ins1(LIR_retd, boxedIndex);
}

LIns* NativeGenerator::generateTypeCheck(LIns* in, Type type, size_t currentByteCodeIndex)
{
#ifndef NDEBUG
    m_out.insComment(".= typecheck start =.");
#endif
    if (type.isBooleanType()) {
#ifdef ESCARGOT_64
        LIns* quadValue = m_out.ins1(LIR_dasq, in);
        LIns* maskedValue = m_out.ins2(LIR_orq, quadValue, m_booleanTagQ);
        LIns* maskedValue2 = m_out.ins2(LIR_subq, quadValue, m_booleanTagQ);
        LIns* checkIfBoolean = m_out.ins2(LIR_leuq, maskedValue2, m_oneI);
        LIns* jumpIfBoolean = m_out.insBranch(LIR_jt, checkIfBoolean, nullptr);
        JIT_LOG(in, "Expected Boolean-typed value, but got this value");
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out.ins0(LIR_label);
        jumpIfBoolean->setTarget(normalPath);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isInt32Type()) {
#ifdef ESCARGOT_64
        LIns* quadValue = m_out.ins1(LIR_dasq, in);
        LIns* maskedValue = m_out.ins2(LIR_andq, quadValue, m_intTagQ);
        LIns* checkIfInt = m_out.ins2(LIR_eqq, maskedValue, m_intTagQ);
        LIns* jumpIfInt = m_out.insBranch(LIR_jt, checkIfInt, nullptr);
        JIT_LOG(in, "Expected Int-typed value, but got this value");
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out.ins0(LIR_label);
        jumpIfInt->setTarget(normalPath);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isDoubleType()) {
#ifdef ESCARGOT_64
        LIns* quadValue = m_out.ins1(LIR_dasq, in);
        LIns* maskedValue = m_out.ins2(LIR_andq, quadValue, m_intTagQ);
        LIns* checkIfNotNumber = m_out.ins2(LIR_eqq, maskedValue, m_zeroQ);
        LIns* exitIfNotNumber = m_out.insBranch(LIR_jt, checkIfNotNumber, nullptr);
        LIns* checkIfInt = m_out.ins2(LIR_eqq, maskedValue, m_intTagQ);
        LIns* jumpIfDouble = m_out.insBranch(LIR_jf, checkIfInt, nullptr);
        LIns* exitPath = m_out.ins0(LIR_label);
        exitIfNotNumber->setTarget(exitPath);
        JIT_LOG(in, "Expected Double-typed value, but got this value");
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out.ins0(LIR_label);
        jumpIfDouble->setTarget(normalPath);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isArrayObjectType() || type.isStringType() || type.isFunctionObjectType()) {
        //ToDo
    } else if (type.isPointerType()) {
#ifdef ESCARGOT_64
        LIns* quadValue = m_out.ins1(LIR_dasq, in);
        LIns* maskedValue = m_out.ins2(LIR_andq, quadValue, m_tagMaskQ);
        LIns* checkIfNotTagged = m_out.ins2(LIR_eqq, maskedValue, m_zeroQ);
        LIns* jumpIfPointer = m_out.insBranch(LIR_jt, checkIfNotTagged, nullptr);
        JIT_LOG(in, "Expected Pointer-typed value, but got this value");
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out.ins0(LIR_label);
        jumpIfPointer->setTarget(normalPath);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isUndefinedType()) {
#ifdef ESCARGOT_64
        LIns* checkIfUndefined = m_out.ins2(LIR_eqq, in, m_undefinedQ);
        LIns* jumpIfUndefined = m_out.insBranch(LIR_jt, checkIfUndefined, nullptr);
        JIT_LOG(in, "Expected undefined value, but got this value");
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out.ins0(LIR_label);
        jumpIfUndefined->setTarget(normalPath);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else {
        std::cout << "Unsupported type in NativeGenerator::generateTypeCheck() : ";
        type.dump(std::cout);
        std::cout << std::endl;
        RELEASE_ASSERT_NOT_REACHED();
    }
#ifndef NDEBUG
    m_out.insComment("'= typecheck ended ='");
#endif
    return in;
}

LIns* NativeGenerator::boxESValue(LIns* unboxedValue, Type type)
{
    if (type.isBooleanType()) {
#ifdef ESCARGOT_64
        LIns* wideUnboxedValue = m_out.ins1(LIR_i2q, unboxedValue);
        LIns* boxedValue = m_out.ins2(LIR_orq, wideUnboxedValue, m_booleanTagQ);
        LIns* boxedValueInDouble = m_out.ins1(LIR_qasd, boxedValue);
        return boxedValueInDouble;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isInt32Type()) {
#ifdef ESCARGOT_64
        LIns* wideUnboxedValue = m_out.ins1(LIR_i2q, unboxedValue);
        LIns* boxedValue = m_out.ins2(LIR_orq, wideUnboxedValue, m_intTagQ);
        LIns* boxedValueInDouble = m_out.ins1(LIR_qasd, boxedValue);
        return boxedValueInDouble;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isDoubleType()) {
#ifdef ESCARGOT_64
        LIns* quadUnboxedValue = m_out.ins1(LIR_dasq, unboxedValue);
        return m_out.ins2(LIR_addq, quadUnboxedValue, m_doubleEncodeOffsetQ);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isPointerType() || type.isUndefinedType() || type.isArrayObjectType() || type.isStringType() || type.isFunctionObjectType()) {
#ifdef ESCARGOT_64
        return unboxedValue;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else {
        std::cout << "Unsupported type in NativeGenerator::boxESValue() : ";
        type.dump(std::cout);
        std::cout << std::endl;
        RELEASE_ASSERT_NOT_REACHED();
    }
}

LIns* NativeGenerator::unboxESValue(LIns* boxedValue, Type type)
{
    if (type.isInt32Type() || type.isBooleanType()) {
#ifdef ESCARGOT_64
        LIns* unboxedValue = m_out.ins2(LIR_andq, boxedValue, m_intTagComplementQ);
        LIns* unboxedValueInInt = m_out.ins1(LIR_q2i, unboxedValue);
        return unboxedValueInInt;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isDoubleType()) {
#ifdef ESCARGOT_64
        LIns* doubleValue = m_out.ins2(LIR_subq, boxedValue, m_doubleEncodeOffsetQ);
        return m_out.ins1(LIR_qasd, doubleValue);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isPointerType() || type.isUndefinedType() || type.isArrayObjectType() || type.isStringType() || type.isFunctionObjectType()) {
#ifdef ESCARGOT_64
        return boxedValue;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else {
        std::cout << "Unsupported type in NativeGenerator::unboxESValue() : ";
        type.dump(std::cout);
        std::cout << std::endl;
        RELEASE_ASSERT_NOT_REACHED();
    }
}

LIns* NativeGenerator::nanojitCodegen(ESIR* ir)
{
    switch(ir->opcode()) {
#ifndef NDEBUG
    #define INIT_ESIR(opcode) \
        opcode##IR* ir##opcode = static_cast<opcode##IR*>(ir); \
        m_out.insComment("# # # # # # # # Opcode " #opcode " # # # # # # # # #");
#else
    #define INIT_ESIR(opcode) \
        opcode##IR* ir##opcode = static_cast<opcode##IR*>(ir);
#endif
    case ESIR::Opcode::ConstantInt:
    {
        INIT_ESIR(ConstantInt);
        return  m_out.insImmI(irConstantInt->value());
    }
    case ESIR::Opcode::ConstantDouble:
    {
        INIT_ESIR(ConstantDouble);
        return m_out.insImmD(irConstantDouble->value());
    }
    case ESIR::Opcode::ConstantString:
    {
        INIT_ESIR(ConstantString);
        return m_out.insImmP(irConstantString->value());
    }
    #define INIT_BINARY_ESIR(opcode) \
        Type leftType = m_graph->getOperandType(ir##opcode->leftIndex()); \
        Type rightType = m_graph->getOperandType(ir##opcode->rightIndex()); \
        LIns* left = getTmpMapping(ir##opcode->leftIndex()); \
        LIns* right = getTmpMapping(ir##opcode->rightIndex());
    case ESIR::Opcode::Int32Plus:
    {
        INIT_ESIR(Int32Plus);
        INIT_BINARY_ESIR(Int32Plus);
        ASSERT(leftType.isInt32Type() && rightType.isInt32Type());
#if 1
        return m_out.ins2(LIR_addi, left, right);
#else
        LIns* add = m_out.ins3(LIR_addjovi, left, right, nullptr;);
#endif
    }
    case ESIR::Opcode::DoublePlus:
    {
        INIT_ESIR(DoublePlus);
        INIT_BINARY_ESIR(DoublePlus);
        if (leftType.isInt32Type())
            left = m_out.ins1(LIR_i2d, left);
        if (rightType.isInt32Type())
            right = m_out.ins1(LIR_i2d, right);
        ASSERT(left->isD() && right->isD());
        return m_out.ins2(LIR_addd, left, right);
    }
    case ESIR::Opcode::StringPlus:
    {
        INIT_ESIR(StringPlus);
        INIT_BINARY_ESIR(StringPlus);
#if 0
        if (!leftType.isStringType())
            left = generateToString(left, leftType);
        if (!rightType.isStringType())
            right = generateToString(right, rightType);
        // TODO
        ASSERT(left->isP() && right->isP());
        //return m_out.ins2(LIR_addd, left, right);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    }
    case ESIR::Opcode::GenericPlus:
    {
        INIT_ESIR(GenericPlus);
        INIT_BINARY_ESIR(GenericPlus);

        LIns* boxedLeft = boxESValue(left, TypeInt32);
        LIns* boxedRight = boxESValue(right, TypeInt32);
        LIns* args[] = {boxedRight, boxedLeft};
        LIns* boxedResult = m_out.insCall(&plusOpCallInfo, args);
        LIns* unboxedResult = unboxESValue(boxedResult, TypeInt32);
        return unboxedResult;
    }
    case ESIR::Opcode::Minus:
    {
        INIT_ESIR(Minus);
        Type leftType = m_graph->getOperandType(irMinus->leftIndex());
        Type rightType = m_graph->getOperandType(irMinus->rightIndex());

        LIns* left = getTmpMapping(irMinus->leftIndex());
        LIns* right = getTmpMapping(irMinus->rightIndex());

        if (leftType.isInt32Type() && rightType.isInt32Type())
            return m_out.ins2(LIR_subi, left, right);
        else if (leftType.isNumberType() && rightType.isNumberType())
            RELEASE_ASSERT_NOT_REACHED();
        else {
            LIns* boxedLeft = boxESValue(left, TypeInt32);
            LIns* boxedRight = boxESValue(right, TypeInt32);
            LIns* args[] = {boxedRight, boxedLeft};
            LIns* boxedResult = m_out.insCall(&minusOpCallInfo, args);
            LIns* unboxedResult = unboxESValue(boxedResult, TypeInt32);
            return unboxedResult;
        }
    }
    case ESIR::Opcode::Int32Multiply:
    {
        INIT_ESIR(Int32Multiply);
        INIT_BINARY_ESIR(Int32Multiply);
#if 1
        LIns* int32Result = m_out.ins3(LIR_muljovi, left, right, nullptr);
        LIns* jumpAlways = m_out.ins2(LIR_j, nullptr, nullptr);
        LIns* labelOverflow = m_out.ins0(LIR_label);
        int32Result->setTarget(labelOverflow);

        JIT_LOG(int32Result, "Int32Multiply : Result is not int32");
        generateOSRExit(irInt32Multiply->targetIndex());

        LIns* labelNoOverflow = m_out.ins0(LIR_label);
        jumpAlways->setTarget(labelNoOverflow);
        return int32Result;
#else
        return m_out.ins2(LIR_muli, left, right);
#endif
    }
    case ESIR::Opcode::DoubleMultiply:
    {
        INIT_ESIR(DoubleMultiply);
        INIT_BINARY_ESIR(DoubleMultiply);
        if (leftType.isInt32Type())
            left = m_out.ins1(LIR_i2d, left);
        if (rightType.isInt32Type())
            right = m_out.ins1(LIR_i2d, right);
        ASSERT(left->isD() && right->isD());
        return m_out.ins2(LIR_muld, left, right);
    }
    case ESIR::Opcode::GenericMultiply:
    {
        // FIXME
        return nullptr;
    }
    case ESIR::Opcode::DoubleDivision:
    {
        INIT_ESIR(DoubleDivision);
        INIT_BINARY_ESIR(DoubleDivision);
        if (leftType.isInt32Type())
            left = m_out.ins1(LIR_i2d, left);
        if (rightType.isInt32Type())
            right = m_out.ins1(LIR_i2d, right);
        ASSERT(left->isD() && right->isD());
        return m_out.ins2(LIR_divd, left, right);
    }
    case ESIR::Opcode::GenericDivision:
    {
        // FIXME
        return nullptr;
    }
    case ESIR::Opcode::BitwiseAnd:
    {
        INIT_ESIR(BitwiseAnd);
        LIns* left = getTmpMapping(irBitwiseAnd->leftIndex());
        LIns* right = getTmpMapping(irBitwiseAnd->rightIndex());
        Type leftType = m_graph->getOperandType(irBitwiseAnd->leftIndex());
        Type rightType = m_graph->getOperandType(irBitwiseAnd->rightIndex());
        if (leftType.isInt32Type() && rightType.isInt32Type())
            return m_out.ins2(LIR_andi, left, right);
        else
            RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::Equal:
    {
        INIT_ESIR(Equal);
        LIns* left = getTmpMapping(irEqual->leftIndex());
        LIns* right = getTmpMapping(irEqual->rightIndex());
        Type leftType = m_graph->getOperandType(irEqual->leftIndex());
        Type rightType = m_graph->getOperandType(irEqual->rightIndex());
        if (leftType.isInt32Type() && rightType.isInt32Type())
            return m_out.ins2(LIR_eqi, left, right);
        else
            RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::GreaterThan:
    {
        INIT_ESIR(GreaterThan);
        LIns* left = getTmpMapping(irGreaterThan->leftIndex());
        LIns* right = getTmpMapping(irGreaterThan->rightIndex());
        Type leftType = m_graph->getOperandType(irGreaterThan->leftIndex());
        Type rightType = m_graph->getOperandType(irGreaterThan->rightIndex());
        if (leftType.isInt32Type() && rightType.isInt32Type())
            return m_out.ins2(LIR_gti, left, right);
        else
            RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::GreaterThanOrEqual:
    {
        INIT_ESIR(GreaterThanOrEqual);
        LIns* left = getTmpMapping(irGreaterThanOrEqual->leftIndex());
        LIns* right = getTmpMapping(irGreaterThanOrEqual->rightIndex());
        Type leftType = m_graph->getOperandType(irGreaterThanOrEqual->leftIndex());
        Type rightType = m_graph->getOperandType(irGreaterThanOrEqual->rightIndex());
        if (leftType.isInt32Type() && rightType.isInt32Type())
            return m_out.ins2(LIR_gei, left, right);
        else
            RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::LessThan:
    {
        INIT_ESIR(LessThan);
        LIns* left = getTmpMapping(irLessThan->leftIndex());
        LIns* right = getTmpMapping(irLessThan->rightIndex());
        Type leftType = m_graph->getOperandType(irLessThan->leftIndex());
        Type rightType = m_graph->getOperandType(irLessThan->rightIndex());
        if (leftType.isInt32Type() && rightType.isInt32Type())
            return m_out.ins2(LIR_lti, left, right);
        else
            RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::LessThanOrEqual:
    {
        INIT_ESIR(LessThanOrEqual);
        LIns* left = getTmpMapping(irLessThanOrEqual->leftIndex());
        LIns* right = getTmpMapping(irLessThanOrEqual->rightIndex());
        Type leftType = m_graph->getOperandType(irLessThanOrEqual->leftIndex());
        Type rightType = m_graph->getOperandType(irLessThanOrEqual->rightIndex());
        if (leftType.isInt32Type() && rightType.isInt32Type())
            return m_out.ins2(LIR_lei, left, right);
        else
            RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::LeftShift:
    {
        INIT_ESIR(LeftShift);
        LIns* left = getTmpMapping(irLeftShift->leftIndex());
        LIns* right = getTmpMapping(irLeftShift->rightIndex());
        Type leftType = m_graph->getOperandType(irLeftShift->leftIndex());
        Type rightType = m_graph->getOperandType(irLeftShift->rightIndex());
        if (leftType.isInt32Type() && rightType.isInt32Type())
            return m_out.ins2(LIR_lshi, left, right);
        else
            RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::SignedRightShift:
    {
        INIT_ESIR(SignedRightShift);
        LIns* left = getTmpMapping(irSignedRightShift->leftIndex());
        LIns* right = getTmpMapping(irSignedRightShift->rightIndex());
        Type leftType = m_graph->getOperandType(irSignedRightShift->leftIndex());
        Type rightType = m_graph->getOperandType(irSignedRightShift->rightIndex());
        if (leftType.isInt32Type() && rightType.isInt32Type())
            return m_out.ins2(LIR_rshi, left, right);
        else
            RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::Jump:
    {
        INIT_ESIR(Jump);
        LIns* jump = m_out.insBranch(LIR_jt, m_true, nullptr);
        if (LIns* label = irJump->targetBlock()->getLabel()) {
            jump->setTarget(label);
        } else {
            irJump->targetBlock()->addJumpOrBranchSource(jump);
        }
        return jump;
    }
    case ESIR::Opcode::Branch:
    {
        INIT_ESIR(Branch);
        LIns* condition = getTmpMapping(irBranch->operandIndex());
        LIns* compare = m_out.ins2(LIR_eqi, condition, m_false);
        LIns* jumpTrue = m_out.insBranch(LIR_jf, compare, nullptr);
        LIns* jumpFalse = m_out.ins2(LIR_j, nullptr, nullptr);
        if (LIns* label = irBranch->falseBlock()->getLabel()) {
            jumpFalse->setTarget(label);
        } else {
            irBranch->trueBlock()->addJumpOrBranchSource(jumpTrue);
            irBranch->falseBlock()->addJumpOrBranchSource(jumpFalse);
        }
        return jumpFalse;
    }
    case ESIR::Opcode::CallJS:
    {
        INIT_ESIR(CallJS);
        LIns* callee = getTmpMapping(irCallJS->calleeIndex());
        LIns* receiver = getTmpMapping(irCallJS->receiverIndex());
        LIns* arguments = m_out.insAlloc(irCallJS->argumentCount() * sizeof(ESValue));
        LIns* argumentCount = m_out.insImmI(irCallJS->argumentCount());
        for (size_t i=0; i<irCallJS->argumentCount(); i++) {
            LIns* argument = getTmpMapping(irCallJS->argumentIndex(i));
            LIns* boxedArgument = boxESValue(argument, m_graph->getOperandType(irCallJS->argumentIndex(i)));
            m_out.insStore(LIR_std, boxedArgument, arguments, i * sizeof(ESValue), 1);
        }
        LIns* args[] = {m_false, argumentCount, arguments, receiver, callee, m_instance};
        LIns* boxedResult = m_out.insCall(&esFunctionObjectCallCallInfo, args);
        return boxedResult;
    }
    case ESIR::Opcode::Return:
    {
        LIns* undefined = m_out.insImmD(0);
        return m_out.ins1(LIR_retd, undefined);
    }
    case ESIR::Opcode::ReturnWithValue:
    {
        INIT_ESIR(ReturnWithValue);
        LIns* returnValue = getTmpMapping(irReturnWithValue->returnIndex());
        LIns* returnESValue = boxESValue(returnValue, m_graph->getOperandType(irReturnWithValue->returnIndex()));
        //JIT_LOG(returnESValue, "Returning this value");
        return m_out.ins1(LIR_retd, returnESValue);
    }
    case ESIR::Opcode::Move:
    {
        INIT_ESIR(Move);
        return getTmpMapping(irMove->sourceIndex());
    }
    case ESIR::Opcode::GetArgument:
    {
        INIT_ESIR(GetArgument);
        LIns* arguments = m_out.insLoad(LIR_ldp, m_context, ExecutionContext::offsetOfArguments(), 1, LOAD_NORMAL);
        LIns* argument = m_out.insLoad(LIR_ldd, arguments, irGetArgument->argumentIndex() * sizeof(ESValue), 1, LOAD_NORMAL);
        // JIT_LOG(argument, "Read this argument");
        return argument;
    }
    case ESIR::Opcode::GetVar:
    {
        INIT_ESIR(GetVar);
        return m_out.insLoad(LIR_ldd, m_stackPtr, irGetVar->varIndex() * sizeof(ESValue), 1, LOAD_NORMAL);
    }
    case ESIR::Opcode::SetVar:
    {
        INIT_ESIR(SetVar);
        LIns* source = getTmpMapping(irSetVar->sourceIndex());
        LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irSetVar->m_targetIndex));
        return m_out.insStore(LIR_std, boxedSource, m_stackPtr, irSetVar->localVarIndex() * sizeof(ESValue), 1);
    }
    case ESIR::Opcode::GetVarGeneric:
    {
        INIT_ESIR(GetVarGeneric);
        LIns* bytecode = m_out.insImmP(irGetVarGeneric->originalGetByIdByteCode());
        LIns* cachedIdentifierCacheInvalidationCheckCount = m_out.insLoad(LIR_ldi, bytecode, offsetof(GetById, m_identifierCacheInvalidationCheckCount), 1, LOAD_NORMAL);
        LIns* instanceIdentifierCacheInvalidationCheckCount = m_out.insLoad(LIR_ldi, m_instance, ESVMInstance::offsetOfIdentifierCacheInvalidationCheckCount(), 1, LOAD_NORMAL);
        LIns* cachedSlot = m_out.insLoad(LIR_ldp, bytecode, offsetof(GetById, m_cachedSlot), 1, LOAD_NORMAL);
        LIns* phi = m_out.insAlloc(sizeof(ESValue));
        LIns* checkIfCacheHit = m_out.ins2(LIR_eqi, instanceIdentifierCacheInvalidationCheckCount, cachedIdentifierCacheInvalidationCheckCount);
        LIns* jumpIfCacheHit = m_out.insBranch(LIR_jt, checkIfCacheHit, nullptr);

        LIns* slowPath = m_out.ins0(LIR_label);
#if 1
        LIns* name = m_out.insImmP(irGetVarGeneric->name());
        LIns* nonAtomicName = m_out.insImmP(irGetVarGeneric->nonAtomicName());
        LIns* args[] = {nonAtomicName, name, m_context};
        LIns* resolvedSlot = m_out.insCall(&contextResolveBindingCallInfo, args);
        LIns* resolvedResult = m_out.insLoad(LIR_ldd, resolvedSlot, 0, 1, LOAD_NORMAL);
        m_out.insStore(LIR_std, resolvedResult, phi, 0 , 1);

        LIns* cachingLookuped = m_out.insStore(LIR_std, resolvedResult, cachedSlot, 0, 1);
        LIns* storeCheckCount = m_out.insStore(LIR_sti, instanceIdentifierCacheInvalidationCheckCount, bytecode, offsetof(GetById, m_identifierCacheInvalidationCheckCount), 1);
        LIns* jumpToJoin = m_out.ins2(LIR_j, nullptr, nullptr);
#else
        JIT_LOG(cachedIdentifierCacheInvalidationCheckCount, "GetVarGeneric Cache Miss");
        generateOSRExit(irGetVarGeneric->targetIndex());
#endif

        LIns* fastPath = m_out.ins0(LIR_label);
        jumpIfCacheHit->setTarget(fastPath);
        LIns* cachedResult = m_out.insLoad(LIR_ldd, cachedSlot, 0, 1, LOAD_NORMAL);
        m_out.insStore(LIR_std, cachedResult, phi, 0 , 1);
#if 1
        LIns* pathJoin = m_out.ins0(LIR_label);
        jumpToJoin->setTarget(pathJoin);
//        LIns* ret = m_out.ins3(LIR_cmovd, checkIfCacheHit, cachedResult, resolvedResult);
        LIns* ret = m_out.insLoad(LIR_ldd, phi, 0, 1, LOAD_NORMAL);
        return ret;
#else
        return cachedResult;
#endif
    }
    case ESIR::Opcode::SetVarGeneric:
    {
        INIT_ESIR(SetVarGeneric);

        LIns* source = getTmpMapping(irSetVarGeneric->sourceIndex());
        LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irSetVarGeneric->m_targetIndex));

        LIns* bytecode = m_out.insImmP(irSetVarGeneric->originalPutByIdByteCode());
        LIns* cachedIdentifierCacheInvalidationCheckCount = m_out.insLoad(LIR_ldi, bytecode, offsetof(PutById, m_identifierCacheInvalidationCheckCount), 1, LOAD_NORMAL);
        LIns* instanceIdentifierCacheInvalidationCheckCount = m_out.insLoad(LIR_ldi, m_instance, ESVMInstance::offsetOfIdentifierCacheInvalidationCheckCount(), 1, LOAD_NORMAL);
        LIns* checkIfCacheHit = m_out.ins2(LIR_eqi, instanceIdentifierCacheInvalidationCheckCount, cachedIdentifierCacheInvalidationCheckCount);
        LIns* jumpIfCacheHit = m_out.insBranch(LIR_jt, checkIfCacheHit, nullptr);

        LIns* slowPath = m_out.ins0(LIR_label);
#if 0
        LIns* name = m_out.insImmP(irSetVarGeneric->name());
        LIns* nonAtomicName = m_out.insImmP(irSetVarGeneric->nonAtomicName());

        LIns* args[] = {nonAtomicName, name, m_context};
        LIns* boxedResultAddress = m_out.insCall(&contextResolveBindingCallInfo, args);
        LIns* checkIfAddressIsNull = m_out.ins2(LIR_eqp, boxedResultAddress, m_zeroP);
        LIns* jumpIfAddressIsNull = m_out.insBranch(LIR_jt, checkIfAddressIsNull, nullptr);

        LIns* store = m_out.insStore(LIR_std, boxedSource, boxedResultAddress, 0, 1);
        LIns* jumpToEnd = m_out.insBranch(LIR_j, nullptr, nullptr);

        LIns* labelDeclareVariable = m_out.ins0(LIR_label);
        jumpIfAddressIsNull->setTarget(labelDeclareVariable);
#if 0
        LIns* args2[] = {boxedSource, /*m_true, m_true, m_true,*/m_globalObject, nonAtomicName};
        m_out.insCall(&objectDefinePropertyOrThrowCallInfo, args2);
#else
        generateOSRExit(irSetVarGeneric->m_targetIndex);
#endif

        LIns* labelEnd = m_out.ins0(LIR_label);
        jumpToEnd->setTarget(labelEnd);

#else
        JIT_LOG(cachedIdentifierCacheInvalidationCheckCount, "SetVarGeneric Cache Miss");
        generateOSRExit(irSetVarGeneric->m_targetIndex);
#endif

        LIns* fastPath = m_out.ins0(LIR_label);
        jumpIfCacheHit->setTarget(fastPath);
        LIns* cachedSlot = m_out.insLoad(LIR_ldp, bytecode, offsetof(PutById, m_cachedSlot), 1, LOAD_NORMAL);
        LIns* storeToCachedSlot = m_out.insStore(LIR_std, boxedSource, cachedSlot, 0, 1);

        return boxedSource;
    }
    case ESIR::Opcode::GetObject:
    {
        INIT_ESIR(GetObject);
        LIns* obj = getTmpMapping(irGetObject->objectIndex());
        if (irGetObject->cachedIndex() < SIZE_MAX) {
            size_t gapToHiddenClassData = escargot::ESObject::offsetOfHiddenClassData();
            LIns* hiddenClassData = m_out.insLoad(LIR_ldd, obj, gapToHiddenClassData, 1, LOAD_NORMAL);
            return m_out.insLoad(LIR_ldd, hiddenClassData, irGetObject->cachedIndex() * sizeof(ESValue), 1, LOAD_NORMAL);
         }
        RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::GetObjectPreComputed:
   {
        INIT_ESIR(GetObjectPreComputed);
        LIns* obj = getTmpMapping(irGetObjectPreComputed->objectIndex());
        if (irGetObjectPreComputed->cachedIndex() < SIZE_MAX) {
#if 0
            LIns* phi = m_out.insAlloc(sizeof(ESValue));
            size_t gapToHiddenClassData = escargot::ESObject::offsetOfHiddenClassData();
            LIns* hiddenClassData = m_out.insLoad(LIR_ldd, obj, gapToHiddenClassData, 1, LOAD_NORMAL);
            LIns* normalResult = m_out.insLoad(LIR_ldd, hiddenClassData, irGetObjectPreComputed->cachedIndex() * sizeof(ESValue), 1, LOAD_NORMAL);
            m_out.insStore(LIR_std, normalResult, phi, 0 , 1);

            LIns* hiddenClass = m_out.insLoad(LIR_ldd, obj, escargot::ESObject::offsetOfHiddenClass(), 1, LOAD_NORMAL);
            LIns* propertyFlagV = m_out.insLoad(LIR_ldd, hiddenClass, escargot::ESHiddenClass::offsetOfPropertyFlagInfo(), 1, LOAD_NORMAL);
            // FIXME : offset can be changed when data structure revised
            LIns* propertyFlags = m_out.insLoad(LIR_lduc2ui, propertyFlagV, irGetObjectPreComputed->cachedIndex() * sizeof(ESHiddenClassPropertyInfo), 1, LOAD_NORMAL);
            LIns* isDataProperty = m_out.ins2(LIR_andi, propertyFlags, m_oneI);

            LIns* checkIfDataProperty = m_out.ins2(LIR_eqi, isDataProperty, m_oneI);
            LIns* jumpIfDataProperty = m_out.insBranch(LIR_jt, checkIfDataProperty, nullptr);

            LIns* args[] = {normalResult, obj};
            LIns* nonDataPropertyResult = m_out.insCall(&resolveNonDataPropertyInfo, args);
            m_out.insStore(LIR_std, nonDataPropertyResult, phi, 0 , 1);

            LIns* labelSimple = m_out.ins0(LIR_label);
            jumpIfDataProperty->setTarget(labelSimple);
            LIns* ret = m_out.insLoad(LIR_ldd, phi, 0, 1, LOAD_NORMAL);
#else
            // FIXME!!!!!!!!
            // This code is simple function call
            LIns* args[] = {m_out.insImmQ(irGetObjectPreComputed->cachedIndex()), obj};
            LIns* ret = m_out.insCall(&resolveNonDataPropertyInfo, args);
#endif
            return ret;
        }
        RELEASE_ASSERT_NOT_REACHED();
   }
    case ESIR::Opcode::GetArrayObject:
    {
        INIT_ESIR(GetArrayObject);
        LIns* obj = getTmpMapping(irGetArrayObject->objectIndex());
        LIns* key = getTmpMapping(irGetArrayObject->propertyIndex());

        ASSERT(m_graph->getOperandType(irGetArrayObject->objectIndex()).isArrayObjectType());
        Type keyType = m_graph->getOperandType(irGetArrayObject->propertyIndex());
            if (keyType.isInt32Type()) {
                size_t gapToVector = escargot::ESArrayObject::offsetOfVectorData();
                LIns* vectorData = m_out.insLoad(LIR_ldd, obj, gapToVector, 1, LOAD_NORMAL);
                LIns* ESValueSize = m_out.insImmI(sizeof(ESValue));
                LIns* offset = m_out.ins2(LIR_muli, key, ESValueSize);
                LIns* newBase = m_out.ins2(LIR_addd, vectorData, offset);
                return m_out.insLoad(LIR_ldd, newBase, 0, 1, LOAD_NORMAL);
             } else
                 RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::GetArrayObjectPreComputed:
    {
        INIT_ESIR(GetArrayObjectPreComputed);
        LIns* obj = getTmpMapping(irGetArrayObjectPreComputed->objectIndex());
        LIns* key = m_out.insImmI(irGetArrayObjectPreComputed->computedIndex());

        ASSERT(m_graph->getOperandType(irGetArrayObjectPreComputed->objectIndex()).isArrayObjectType());
        size_t gapToVector = escargot::ESArrayObject::offsetOfVectorData();
        LIns* vectorData = m_out.insLoad(LIR_ldd, obj, gapToVector, 1, LOAD_NORMAL);
        LIns* ESValueSize = m_out.insImmI(sizeof(ESValue));
        LIns* offset = m_out.ins2(LIR_muli, key, ESValueSize);
        LIns* newBase = m_out.ins2(LIR_addd, vectorData, offset);
        return m_out.insLoad(LIR_ldd, newBase, 0, 1, LOAD_NORMAL);
    }
    case ESIR::Opcode::PutInObject:
    {
        INIT_ESIR(PutInObject);
        LIns* obj = getTmpMapping(irPutInObject->objectIndex());
        LIns* prop = getTmpMapping(irPutInObject->propertyIndex());
        LIns* source = getTmpMapping(irPutInObject->sourceIndex());

        Type objType = m_graph->getOperandType(irPutInObject->objectIndex());
        Type propType = m_graph->getOperandType(irPutInObject->propertyIndex());
        Type sourceType = m_graph->getOperandType(irPutInObject->sourceIndex());

        if (objType.isArrayObjectType()) {
            if (propType.isInt32Type()) {
                if (sourceType.isBooleanType()) {
                    LIns* boxedProp = boxESValue(prop, TypeInt32);
                    LIns* boxedSrc = boxESValue(source, TypeBoolean);
                    LIns* args[] = {boxedSrc, boxedProp, obj};
                    return m_out.insCall(&ESObjectSetOpCallInfo, args);
                } else if (sourceType.isInt32Type()) {
                    LIns* boxedProp = boxESValue(prop, TypeInt32);
                    LIns* boxedSrc = boxESValue(source, TypeInt32);
                    LIns* args[] = {boxedSrc, boxedProp, obj};
                    return m_out.insCall(&ESObjectSetOpCallInfo, args);
                } else if (sourceType.isDoubleType()) {
                    LIns* boxedProp = boxESValue(prop, TypeInt32);
                    LIns* boxedSrc = boxESValue(source, TypeDouble);
                    LIns* args[] = {boxedSrc, boxedProp, obj};
                    return m_out.insCall(&ESObjectSetOpCallInfo, args);
                } else {
                    LIns* boxedProp = boxESValue(prop, TypeInt32);
                    LIns* args[] = {source, boxedProp, obj};
                    return m_out.insCall(&ESObjectSetOpCallInfo, args);
                }
             }
        }

        LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irPutInObject->targetIndex()));
        return m_out.insStore(LIR_std, boxedSource, (LIns*) irPutInObject->cachedHiddenClass(), irPutInObject->cachedIndex() * sizeof(ESValue), 1);
    }
    case ESIR::Opcode::ToNumber:
    {
        INIT_ESIR(ToNumber);
        LIns* source = getTmpMapping(irToNumber->sourceIndex());
        Type srcType = m_graph->getOperandType(irToNumber->sourceIndex());
        if (srcType.isInt32Type() || srcType.isDoubleType()) {
            return source;
        } else
            RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::Increment:
    {
        INIT_ESIR(Increment);
        LIns* source = getTmpMapping(irIncrement->sourceIndex());
        Type srcType = m_graph->getOperandType(irIncrement->sourceIndex());
        if (srcType.isInt32Type()) {
            LIns* one = m_out.insImmI(1);
            LIns* ret = m_out.ins2(LIR_addi, source, one);

//            JIT_LOG(ret);

            return ret;
        } else if (srcType.isDoubleType()) {
            LIns* one = m_out.insImmD(1);
            return m_out.ins2(LIR_addd, source, one);
        } else
            RELEASE_ASSERT_NOT_REACHED();
    }
    case ESIR::Opcode::UnaryMinus:
        {
            INIT_ESIR(UnaryMinus);
            LIns* source = getTmpMapping(irUnaryMinus->sourceIndex());
            Type srcType = m_graph->getOperandType(irUnaryMinus->sourceIndex());
            if (srcType.isInt32Type()) {
                return m_out.ins1(LIR_negi, source);
            } else if (srcType.isDoubleType()) {
                return m_out.ins1(LIR_negd, source);
            } else
                RELEASE_ASSERT_NOT_REACHED();
        }
    default:
    {
        return nullptr;
    }
    #undef INIT_ESIR
    }
}

void NativeGenerator::nanojitCodegen(ESVMInstance* instance)
{
    m_out.ins0(LIR_start);
    for (int i = 0; i < nanojit::NumSavedRegs; ++i)
        m_out.insParam(i, 1);
    m_instance = m_out.insImmP(instance);
    m_context = m_out.insLoad(LIR_ldp, m_instance, ESVMInstance::offsetOfCurrentExecutionContext(), 1, LOAD_NORMAL); // FIXME generate this only if really needed
    m_stackPtr = m_out.insAlloc(m_graph->tempRegisterSize() * sizeof(ESValue));
    m_globalObject = m_out.insLoad(LIR_ldp, m_instance, ESVMInstance::offsetOfGlobalObject(), 1, LOAD_NORMAL); // FIXME generate this only if really needed

#ifdef ESCARGOT_64
    m_tagMaskQ = m_out.insImmQ(TagMask);
    m_booleanTagQ = m_out.insImmQ(TagBitTypeOther | TagBitBool);
    m_intTagQ = m_out.insImmQ(TagTypeNumber);
    m_intTagComplementQ = m_out.insImmQ(~TagTypeNumber);
    m_doubleEncodeOffsetQ = m_out.insImmQ(DoubleEncodeOffset);
    m_undefinedQ = m_out.insImmQ(ValueUndefined);
#endif
    m_zeroQ = m_out.insImmQ(0);
    m_zeroP = m_out.insImmP(0);
    m_oneI = m_out.insImmI(1);
    m_zeroI = m_out.insImmI(0);
    m_true = m_oneI;
    m_false = m_zeroI;

    // JIT_LOG(m_true, "Start executing JIT function");

    // Generate code for each IRs
    for (size_t i = 0; i < m_graph->basicBlockSize(); i++) {
        ESBasicBlock* block = m_graph->basicBlock(i);
        block->setLabel(m_out.ins0(LIR_label));
        for (size_t j = 0; j < block->instructionSize(); j++) {
            ESIR* ir = block->instruction(j);
            LIns* generatedLIns = nanojitCodegen(ir);
            if (!generatedLIns)
                printf("ERROR: Cannot generate code for JIT IR `%s`\n", ir->getOpcodeName());
            if (ir->isValueLoadedFromHeap()) {
                Type type = m_graph->getOperandType(ir->m_targetIndex);
                generatedLIns = generateTypeCheck(generatedLIns, type, ir->m_targetIndex);
                generatedLIns = unboxESValue(generatedLIns, type);
            }
            if (ir->m_targetIndex >= 0) {
                setTmpMapping(ir->m_targetIndex, generatedLIns);
            }
        }
    }

    // Link jump addresses
    for (size_t i = 0; i < m_graph->basicBlockSize(); i++) {
        ESBasicBlock* block = m_graph->basicBlock(i);
        for (size_t j = 0; j < block->m_jumpOrBranchSources.size(); j++)
            block->m_jumpOrBranchSources[j]->setTarget(block->getLabel());
    }

    SideExit* exit = new SideExit();
    memset(exit, 0, sizeof(SideExit));
    exit->from = m_f;
    exit->target = NULL;

    GuardRecord *rec = new GuardRecord();
    memset(rec, 0, sizeof(GuardRecord));
    rec->exit = exit;
    exit->addGuard(rec);

    m_f->lastIns = m_out.insGuard(LIR_x, nullptr, rec);
}

JITFunction NativeGenerator::nativeCodegen() {

    m_assm->compile(m_f, *m_alloc, false verbose_only(, m_f->lirbuf->printer));
    if (m_assm->error() != None) {
        if (ESVMInstance::currentInstance()->m_profile)
            printf("error compiling fragment\n");
        return nullptr;
    }

    return reinterpret_cast<JITFunction>(m_f->code());
}

JITFunction generateNativeFromIR(ESGraph* graph, ESVMInstance* instance)
{
    NativeGenerator gen(graph);

    unsigned long time1 = ESVMInstance::currentInstance()->tickCount();
    gen.nanojitCodegen(instance);
    unsigned long time2 = ESVMInstance::currentInstance()->tickCount();
    JITFunction function = gen.nativeCodegen();
    unsigned long time3 = ESVMInstance::currentInstance()->tickCount();
    if (ESVMInstance::currentInstance()->m_profile)
        printf("JIT Compilation Took %lfms, %lfms each for nanojit/native generation\n",
                (time2-time1)/1000.0, (time3-time2)/1000.0);

    return function;
}

JITFunction addDouble()
{
    unsigned long start = ESVMInstance::currentInstance()->tickCount();

    using namespace nanojit;
    LogControl lc;
#ifndef NDEBUG
    lc.lcbits = LC_ReadLIR | LC_Native;
#else
    lc.lcbits = 0;
#endif

    Config config;
    Allocator *alloc = new Allocator();
    CodeAlloc *codeAlloc = new CodeAlloc(&config);
    Assembler *assm = new  Assembler(*codeAlloc, *alloc, *alloc, &lc, config);
    LirBuffer *buf = new LirBuffer(*alloc);
#ifdef DEBUG
    buf->printer = new LInsPrinter(*alloc, 1);
#endif

    Fragment *f = new Fragment(NULL verbose_only(, 0));
    f->lirbuf = buf;

    // Create a LIR writer
    LirBufWriter out(buf, config);

    // Write a few LIR instructions to the buffer: add the first parameter
    // to the constant 2.
    out.ins0(LIR_start);
    LIns *two = out.insImmD(2);
    LIns *firstParam = out.insParam(0, 0);
    LIns *result = out.ins2(LIR_addd, firstParam, two);
    out.ins1(LIR_retd, result);

    SideExit* exit = new SideExit();
    memset(exit, 0, sizeof(SideExit));
    exit->from = f;
    exit->target = NULL;

    GuardRecord *rec = new GuardRecord();
    memset(rec, 0, sizeof(GuardRecord));
    rec->exit = exit;
    exit->addGuard(rec);

    f->lastIns = out.insGuard(LIR_x, nullptr, rec);

    // Compile the fragment.
    assm->compile(f, *alloc, false verbose_only(, f->lirbuf->printer));
    if (assm->error() != None) {
        fprintf(stderr, "error compiling fragment\n");
        return nullptr;
    }
    printf("Compilation successful.\n");

    unsigned long end = ESVMInstance::currentInstance()->tickCount();
    printf("JIT Compilation Took %lf ms\n",(end-start)/1000.0);

    return reinterpret_cast<JITFunction>(f->code());
}

int nanoJITTest()
{
    unsigned long start = ESVMInstance::currentInstance()->tickCount();

    using namespace nanojit;
    LogControl lc;
#ifdef DEBUG
    lc.lcbits = LC_ReadLIR | LC_Native;
#else
    lc.lcbits = 0;
#endif

#if 0
    // Set up the basic Nanojit objects.
    Allocator *alloc = new VMAllocator();
    CodeAlloc *codeAlloc = new CodeAlloc();
    Assembler *assm = new (&gc) Assembler(*codeAlloc, *alloc, &core, &lc);
    Fragmento *fragmento;
    LirBuffer *buf = new (*alloc) LirBuffer(*alloc);
#else
    Config config;
    Allocator *alloc = new Allocator();
    CodeAlloc *codeAlloc = new CodeAlloc(&config);
    Assembler *assm = new Assembler(*codeAlloc, *alloc, *alloc, &lc, config);
    //Fragmento *fragmento;
    LirBuffer *buf = new LirBuffer(*alloc);
#ifdef DEBUG
    buf->printer = new LInsPrinter(*alloc, 1);
#endif
#endif

#if 0
    // Create a Fragment to hold some native code.
    Fragment *f = fragmento->getAnchor((void *)0xdeadbeef);
    f->lirbuf = buf;
    f->root = f;
#else
    Fragment *f = new Fragment(NULL verbose_only(, 0));
    f->lirbuf = buf;
#endif

    // Create a LIR writer
    LirBufWriter out(buf, config);

    // Write a few LIR instructions to the buffer: add the first parameter
    // to the constant 2.
    out.ins0(LIR_start);
    LIns *two = out.insImmI(2);
    LIns *firstParam = out.insParam(0, 0);
    LIns *result = out.ins2(LIR_addi, firstParam, two);
    out.ins1(LIR_reti, result);

#if 0
    // Emit a LIR_loop instruction.  It won't be reached, but there's
    // an assertion in Nanojit that trips if a fragment doesn't end with
    // a guard (a bug in Nanojit).
    LIns *rec_ins = out.insSkip(sizeof(GuardRecord) + sizeof(SideExit));
    GuardRecord *guard = (GuardRecord *) rec_ins->payload();
    memset(guard, 0, sizeof(*guard));
    SideExit *exit = (SideExit *)(guard + 1);
    guard->exit = exit;
    guard->exit->target = f;
    f->lastIns = out.insGuard(LIR_loop, out.insImm(1), rec_ins);
#else
    SideExit* exit = new SideExit();
    memset(exit, 0, sizeof(SideExit));
    exit->from = f;
    exit->target = NULL;

    GuardRecord *rec = new GuardRecord();
    memset(rec, 0, sizeof(GuardRecord));
    rec->exit = exit;
    exit->addGuard(rec);

    f->lastIns = out.insGuard(LIR_x, nullptr, rec);
#endif

    // Compile the fragment.
    assm->compile(f, *alloc, false verbose_only(, f->lirbuf->printer));
    if (assm->error() != None) {
        fprintf(stderr, "error compiling fragment\n");
        return 1;
    }
    printf("Compilation successful.\n");

    unsigned long end = ESVMInstance::currentInstance()->tickCount();
    printf("Took %lf ms\n",(end-start)/1000.0);

    typedef int32_t (*AddTwoFn)(int32_t);
    AddTwoFn fn = reinterpret_cast<AddTwoFn>(f->code());
    printf("2 + 5 = %d\n", fn(5));

    return 0;
}

}}
#endif
