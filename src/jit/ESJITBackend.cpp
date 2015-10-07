#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITBackend.h"

#include "ESJIT.h"
#include "ESIR.h"
#include "ESIRType.h"
#include "ESGraph.h"
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
#ifndef NDEBUG
CallInfo logIntCallInfo = CI(jitLogIntOperation, CallInfo::typeSig1(ARGTYPE_V, ARGTYPE_I));
CallInfo logDoubleCallInfo = CI(jitLogDoubleOperation, CallInfo::typeSig1(ARGTYPE_V, ARGTYPE_D));
CallInfo logPointerCallInfo = CI(jitLogPointerOperation, CallInfo::typeSig1(ARGTYPE_V, ARGTYPE_P));
#define JIT_LOG_I(arg) { LIns* args[] = {arg}; m_out.insCall(&logIntCallInfo, args); }
#define JIT_LOG_D(arg) { LIns* args[] = {arg}; m_out.insCall(&logDoubleCallInfo, args); }
#define JIT_LOG_P(arg) { LIns* args[] = {arg}; m_out.insCall(&logPointerCallInfo, args); }
#define JIT_LOG(arg) { \
    if (arg->isI()) JIT_LOG_I(arg); \
    if (arg->isD()) JIT_LOG_D(arg); \
    if (arg->isP()) JIT_LOG_P(arg); \
}
#else
#define JIT_LOG_I(arg)
#define JIT_LOG_D(arg)
#define JIT_LOG_P(arg)
#define JIT_LOG(arg)
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
    return m_out.ins1(LIR_retq, bytecode); // FIXME returning int as quad
}

LIns* NativeGenerator::generateTypeCheck(LIns* in, Type type, size_t currentByteCodeIndex)
{
#ifndef NDEBUG
    m_out.insComment(".= typecheck start =.");
#endif
    if (type.isInt32Type()) {
#ifdef ESCARGOT_64
        LIns* quadValue = m_out.ins1(LIR_dasq, in);
        LIns* maskedValue = m_out.ins2(LIR_andq, quadValue, m_intTagQ);
        LIns* checkIfInt = m_out.ins2(LIR_eqq, maskedValue, m_intTagQ);
        LIns* jumpIfInt = m_out.insBranch(LIR_jt, checkIfInt, nullptr);
        JIT_LOG(in);
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out.ins0(LIR_label);
        jumpIfInt->setTarget(normalPath);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isArrayObjectType()) {
        //ToDo
    } else if (type.isPointerType()) {
#ifdef ESCARGOT_64
        LIns* quadValue = m_out.ins1(LIR_dasq, in);
        LIns* maskedValue = m_out.ins2(LIR_andq, quadValue, m_tagMaskQ);
        LIns* checkIfNotTagged = m_out.ins2(LIR_eqq, maskedValue, m_zeroQ);
        LIns* jumpIfPointer = m_out.insBranch(LIR_jt, checkIfNotTagged, nullptr);
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
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out.ins0(LIR_label);
        jumpIfUndefined->setTarget(normalPath);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else {
        RELEASE_ASSERT_NOT_REACHED();
    }
#ifndef NDEBUG
    m_out.insComment("'= typecheck ended ='");
#endif
    return in;
}

LIns* NativeGenerator::boxESValue(LIns* unboxedValue, Type type)
{
    if (type.isInt32Type() || type.isBooleanType()) {
#ifdef ESCARGOT_64
        LIns* wideUnboxedValue = m_out.ins1(LIR_i2q, unboxedValue);
        LIns* boxedValue = m_out.ins2(LIR_orq, wideUnboxedValue, m_intTagQ);
        LIns* boxedValueInDouble = m_out.ins1(LIR_qasd, boxedValue);
        return boxedValueInDouble;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isPointerType() || type.isUndefinedType() || type.isArrayObjectType()) {
#ifdef ESCARGOT_64
        return unboxedValue;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else {
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
        LIns* doubleOffset = m_out.insImmQ(DoubleEncodeOffset);
        LIns* doubleValue = m_out.ins2(LIR_subq, boxedValue, doubleOffset);
        LIns* intValue = m_out.ins1(LIR_q2i, doubleValue);
        return intValue;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isPointerType() || type.isUndefinedType() || type.isArrayObjectType()) {
#ifdef ESCARGOT_64
        return boxedValue;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else {
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
    case ESIR::Opcode::GenericPlus:
    {
        INIT_ESIR(GenericPlus);
        Type leftType = m_graph->getOperandType(irGenericPlus->leftIndex());
        Type rightType = m_graph->getOperandType(irGenericPlus->rightIndex());

        LIns* left = getTmpMapping(irGenericPlus->leftIndex());
        LIns* right = getTmpMapping(irGenericPlus->rightIndex());

        if (leftType.isInt32Type() && rightType.isInt32Type())
            return m_out.ins2(LIR_addi, left, right);
        else if (leftType.isNumberType() && rightType.isNumberType())
            RELEASE_ASSERT_NOT_REACHED();
        else {
            LIns* boxedLeft = boxESValue(left, TypeInt32);
            LIns* boxedRight = boxESValue(right, TypeInt32);
            LIns* args[] = {boxedRight, boxedLeft};
            LIns* boxedResult = m_out.insCall(&plusOpCallInfo, args);
            LIns* unboxedResult = unboxESValue(boxedResult, TypeInt32);
            return unboxedResult;
        }
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
        LIns* returnESValue = m_out.ins1(LIR_i2q, returnValue);
        return m_out.ins1(LIR_retq, returnESValue);
    }
    case ESIR::Opcode::GetArgument:
    {
        INIT_ESIR(GetArgument);
        LIns* arguments = m_out.insLoad(LIR_ldp, m_context, ExecutionContext::offsetOfArguments(), 1, LOAD_NORMAL);
        LIns* argument = m_out.insLoad(LIR_ldp, arguments, irGetArgument->argumentIndex() * sizeof(ESValue), 1, LOAD_NORMAL);
#if 0
        JIT_LOG_D(argument);
#endif
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
        LIns* name = m_out.insImmP(irGetVarGeneric->name());
        LIns* nonAtomicName = m_out.insImmP(irGetVarGeneric->nonAtomicName());
        LIns* args[] = {nonAtomicName, name, m_context};
        LIns* boxedResultAddress = m_out.insCall(&contextResolveBindingCallInfo, args);
        LIns* boxedResult = m_out.insLoad(LIR_ldd, boxedResultAddress, 0, 1, LOAD_NORMAL);
        return boxedResult;
    }
    case ESIR::Opcode::SetVarGeneric:
    {
        INIT_ESIR(SetVarGeneric);
        LIns* name = m_out.insImmP(irSetVarGeneric->name());
        LIns* nonAtomicName = m_out.insImmP(irSetVarGeneric->nonAtomicName());

        LIns* args[] = {nonAtomicName, name, m_context};
        LIns* boxedResultAddress = m_out.insCall(&contextResolveBindingCallInfo, args);
        LIns* checkIfAddressIsNull = m_out.ins2(LIR_eqp, boxedResultAddress, m_zeroP);
        LIns* jumpIfAddressIsNull = m_out.insBranch(LIR_jt, checkIfAddressIsNull, nullptr);

        LIns* source = getTmpMapping(irSetVarGeneric->sourceIndex());
        LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irSetVarGeneric->m_targetIndex));
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

        return boxedSource;
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
            if (propType.isInt32Type() && sourceType.isInt32Type()) {
                LIns* boxedProp = boxESValue(prop, TypeInt32);
                LIns* boxedSrc = boxESValue(source, TypeInt32);
                LIns* args[] = {boxedSrc, boxedProp, obj};
                return m_out.insCall(&ESObjectSetOpCallInfo, args);
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
            return m_out.ins2(LIR_addi, source, one);
        } else if (srcType.isDoubleType()) {
            LIns* one = m_out.insImmD(1);
            return m_out.ins2(LIR_addd, source, one);
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

void NativeGenerator::nanojitCodegen()
{
    m_out.ins0(LIR_start);

    for (int i = 0; i < nanojit::NumSavedRegs; ++i)
        m_out.insParam(i, 1);

    m_instance = m_out.insParam(0, 0);
    m_context = m_out.insLoad(LIR_ldp, m_instance, ESVMInstance::offsetOfCurrentExecutionContext(), 1, LOAD_NORMAL); // FIXME generate this only if really needed
    m_stackPtr = m_out.insAlloc(m_graph->tempRegisterSize() * sizeof(ESValue));
    m_globalObject = m_out.insLoad(LIR_ldp, m_instance, ESVMInstance::offsetOfGlobalObject(), 1, LOAD_NORMAL); // FIXME generate this only if really needed

    m_tagMaskQ = m_out.insImmQ(TagMask);
    m_intTagQ = m_out.insImmQ(TagTypeNumber);
    m_intTagComplementQ = m_out.insImmQ(~TagTypeNumber);
    m_undefinedQ = m_out.insImmQ(ValueUndefined);
    m_zeroQ = m_out.insImmQ(0);
    m_zeroP = m_out.insImmP(0);
    m_oneI = m_out.insImmI(1);
    m_zeroI = m_out.insImmI(0);
    m_true = m_oneI;
    m_false = m_zeroI;

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

JITFunction generateNativeFromIR(ESGraph* graph)
{
    NativeGenerator gen(graph);

    unsigned long time1 = ESVMInstance::currentInstance()->tickCount();
    gen.nanojitCodegen();
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
