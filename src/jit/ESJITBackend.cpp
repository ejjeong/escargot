#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITBackend.h"

#include "ESJIT.h"
#include "ESIR.h"
#include "ESIRType.h"
#include "ESGraph.h"
#include "bytecode/ByteCode.h"
#include "bytecode/ByteCodeOperations.h"
#include "ESJITOperations.h"
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

#ifdef ESCARGOT_64
#define ARGTYPE_E ARGTYPE_Q
#define LIR_lde LIR_ldq
#define LIR_ste LIR_stq
#define LIR_rete LIR_retq
#else
#define ARGTYPE_E ARGTYPE_D
#define LIR_lde LIR_ldd
#define LIR_ste LIR_std
#define LIR_rete LIR_retd
#endif

/* CallInfo */
CallInfo workaroundForSavingDoubleCallInfo = CI(workaroundForSavingDouble, CallInfo::typeSig1(ARGTYPE_E, ARGTYPE_D));

CallInfo getByIndexWithActivationOpCallInfo = CI(getByIndexWithActivationOp, CallInfo::typeSig3(ARGTYPE_E, ARGTYPE_P, ARGTYPE_I, ARGTYPE_I));
CallInfo setByIndexWithActivationOpCallInfo = CI(setByIndexWithActivationOp, CallInfo::typeSig4(ARGTYPE_V, ARGTYPE_P, ARGTYPE_I, ARGTYPE_I, ARGTYPE_E));

CallInfo getByGlobalIndexOpCallInfo = CI(getByGlobalIndexOp, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_P, ARGTYPE_P));
CallInfo setByGlobalIndexOpCallInfo = CI(setByGlobalIndexOp, CallInfo::typeSig3(ARGTYPE_V, ARGTYPE_P, ARGTYPE_P, ARGTYPE_E));

CallInfo getByIdWithoutExceptionOpCallInfo = CI(getByIdWithoutExceptionOp, CallInfo::typeSig3(ARGTYPE_E, ARGTYPE_P, ARGTYPE_P, ARGTYPE_P));

/* CallInfo: Binary/Unary operations */
CallInfo plusOpCallInfo = CI(plusOp, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));
CallInfo minusOpCallInfo = CI(minusOp, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));
CallInfo bitwiseOrOpCallInfo = CI(bitwiseOrOp, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));
CallInfo bitwiseXorOpCallInfo = CI(bitwiseXorOp, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));
CallInfo leftShiftOpCallInfo = CI(leftShiftOp, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));
CallInfo signedRightShiftOpCallInfo = CI(signedRightShiftOp, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));
CallInfo unsignedRightShiftOpCallInfo = CI(unsignedRightShiftOp, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));
CallInfo bitwiseNotOpCallInfo = CI(bitwiseNotOp, CallInfo::typeSig1(ARGTYPE_E, ARGTYPE_E));
CallInfo logicalNotOpCallInfo = CI(logicalNotOp, CallInfo::typeSig1(ARGTYPE_E, ARGTYPE_E));
CallInfo typeOfOpCallInfo = CI(typeOfOp, CallInfo::typeSig1(ARGTYPE_E, ARGTYPE_E));
CallInfo equalOpCallInfo = CI(equalOp, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));
CallInfo strictEqualOpCallInfo = CI(strictEqualOp, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));
CallInfo lessThanOpCallInfo = CI(lessThanOp, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));

/* CallInfo */
CallInfo contextResolveBindingCallInfo = CI(contextResolveBinding, CallInfo::typeSig2(ARGTYPE_P, ARGTYPE_P, ARGTYPE_P));
CallInfo contextResolveThisBindingCallInfo = CI(contextResolveThisBinding, CallInfo::typeSig1(ARGTYPE_E, ARGTYPE_P));
CallInfo setVarContextResolveBindingCallInfo = CI(setVarContextResolveBinding, CallInfo::typeSig2(ARGTYPE_P, ARGTYPE_P, ARGTYPE_P));
CallInfo setVarDefineDataPropertyCallInfo = CI(setVarDefineDataProperty, CallInfo::typeSig4(ARGTYPE_V, ARGTYPE_P, ARGTYPE_P, ARGTYPE_P, ARGTYPE_E));
CallInfo esFunctionObjectCallCallInfo = CI(esFunctionObjectCall, CallInfo::typeSig5(ARGTYPE_E, ARGTYPE_P, ARGTYPE_E, ARGTYPE_P, ARGTYPE_I, ARGTYPE_I));
CallInfo esFunctionObjectCallWithReceiverCallInfo = CI(esFunctionObjectCallWithReceiver, CallInfo::typeSig6(ARGTYPE_E, ARGTYPE_P, ARGTYPE_E, ARGTYPE_E, ARGTYPE_P, ARGTYPE_I, ARGTYPE_I));
CallInfo newOpCallInfo = CI(newOp, CallInfo::typeSig5(ARGTYPE_E, ARGTYPE_P, ARGTYPE_P, ARGTYPE_E, ARGTYPE_P, ARGTYPE_I));
CallInfo evalCallCallInfo = CI(evalCall, CallInfo::typeSig4(ARGTYPE_E, ARGTYPE_P, ARGTYPE_P, ARGTYPE_I, ARGTYPE_P));
CallInfo getObjectOpCallInfo = CI(getObjectOp, CallInfo::typeSig3(ARGTYPE_E, ARGTYPE_E, ARGTYPE_E, ARGTYPE_P));
CallInfo getObjectPreComputedCaseOpCallInfo = CI(getObjectPreComputedCaseOp, CallInfo::typeSig3(ARGTYPE_E, ARGTYPE_E, ARGTYPE_P, ARGTYPE_P));
CallInfo getObjectPreComputedCaseLastPartOpCallInfo = CI(getObjectPreComputedCaseOpLastPart, CallInfo::typeSig3(ARGTYPE_E, ARGTYPE_P, ARGTYPE_P, ARGTYPE_Q));
CallInfo setObjectOpCallInfo = CI(setObjectOp, CallInfo::typeSig3(ARGTYPE_V, ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));
CallInfo setObjectPreComputedCaseOpCallInfo = CI(setObjectPreComputedOp, CallInfo::typeSig4(ARGTYPE_V, ARGTYPE_E, ARGTYPE_P, ARGTYPE_P, ARGTYPE_E));
CallInfo generateToStringCallInfo = CI(generateToString, CallInfo::typeSig1(ARGTYPE_P, ARGTYPE_E));
CallInfo concatTwoStringsCallInfo = CI(concatTwoStrings, CallInfo::typeSig2(ARGTYPE_P, ARGTYPE_P, ARGTYPE_P));
CallInfo createObjectCallInfo = CI(createObject, CallInfo::typeSig1(ARGTYPE_E, ARGTYPE_I));
CallInfo createArrayCallInfo = CI(createArr, CallInfo::typeSig1(ARGTYPE_E, ARGTYPE_I));
CallInfo createFunctionCallInfo = CI(createFunction, CallInfo::typeSig2(ARGTYPE_E, ARGTYPE_P, ARGTYPE_P));
CallInfo initObjectCallInfo = CI(initObject, CallInfo::typeSig3(ARGTYPE_V, ARGTYPE_E, ARGTYPE_E, ARGTYPE_E));
CallInfo throwCallInfo = CI(throwOp, CallInfo::typeSig1(ARGTYPE_V, ARGTYPE_E));
CallInfo getEnumerablObjectDataCallInfo = CI(getEnumerablObjectData, CallInfo::typeSig1(ARGTYPE_P, ARGTYPE_E));
CallInfo keySizeCallInfo = CI(keySize, CallInfo::typeSig1(ARGTYPE_I, ARGTYPE_P));
CallInfo getEnumerationKeyCallInfo = CI(getEnumerationKey, CallInfo::typeSig1(ARGTYPE_E, ARGTYPE_P));
CallInfo toBooleanCallInfo = CI(toBoolean, CallInfo::typeSig1(ARGTYPE_I, ARGTYPE_E));
#ifndef NDEBUG
CallInfo logIntCallInfo = CI(jitLogIntOperation, CallInfo::typeSig2(ARGTYPE_V, ARGTYPE_I, ARGTYPE_P));
CallInfo logDoubleCallInfo = CI(jitLogDoubleOperation, CallInfo::typeSig2(ARGTYPE_V, ARGTYPE_D, ARGTYPE_P));
CallInfo logPointerCallInfo = CI(jitLogPointerOperation, CallInfo::typeSig2(ARGTYPE_V, ARGTYPE_P, ARGTYPE_P));
#define JIT_LOG_I(arg, msg) { LIns* args[] = { m_out->insImmP(msg), arg}; m_out->insCall(&logIntCallInfo, args); }
#define JIT_LOG_D(arg, msg) { LIns* args[] = { m_out->insImmP(msg), arg}; m_out->insCall(&logDoubleCallInfo, args); }
#define JIT_LOG_P(arg, msg) { LIns* args[] = { m_out->insImmP(msg), arg}; m_out->insCall(&logPointerCallInfo, args); }
#define JIT_LOG(arg, msg) { \
    if (arg->isI()) { \
        JIT_LOG_I(arg, msg); \
    } else if (arg->isD()) { \
        JIT_LOG_D(arg, msg); \
    } else if (arg->isP()) { \
        JIT_LOG_P(arg, msg); \
    } else \
        RELEASE_ASSERT_NOT_REACHED(); \
}
#else
#define JIT_LOG_I(arg, msg)
#define JIT_LOG_D(arg, msg)
#define JIT_LOG_P(arg, msg)
#define JIT_LOG(arg, msg)
#endif

NativeGenerator::NativeGenerator(ESGraph* graph)
    : m_graph(graph),
    m_tmpToLInsMapping(graph->tempRegisterSize()),
    m_alloc(new Allocator()),
    m_codeAlloc(new CodeAlloc(&m_config)),
    m_assm(new Assembler(*m_codeAlloc, *m_alloc, *m_alloc, &m_lc, m_config)),
    m_buf(new LirBuffer(*m_alloc)),
    m_f(new Fragment(NULL verbose_only(, 0))),
    m_out(new LirBufWriter(m_buf, m_config))
{
#ifndef NDEBUG
    if (ESVMInstance::currentInstance()->m_verboseJIT)
        m_lc.lcbits = LC_ReadLIR | LC_Native;
    else
        m_lc.lcbits = 0;
    m_buf->printer = new LInsPrinter(*m_alloc, 1);

    // Default Writer Pipeline
    m_out = new ValidateWriter(m_out, m_buf->printer, "[validate]");
    m_out = new LirWriter(m_out);

    // Selectable Writer Pipeline
    if (ESVMInstance::currentInstance()->m_useVerboseWriter)
        m_out = new VerboseWriter(*m_alloc, m_out, m_buf->printer, &m_lc, "[verbose]");

#else
    m_lc.lcbits = 0;
#endif

    if (ESVMInstance::currentInstance()->m_useExprFilter)
        m_out = new ExprFilter(m_out);
    if (ESVMInstance::currentInstance()->m_useCseFilter)
        m_out = new CseFilter(m_out, 1, *m_alloc);
    m_f->lirbuf = m_buf;
}

NativeGenerator::~NativeGenerator()
{
    // TODO : separate long-lived and short-lived data structures
#ifndef NDEBUG
    delete m_buf->printer;
#endif
    delete m_alloc;
    // delete m_codeAlloc;
    delete m_assm;
    delete m_buf;
    delete m_f;
    delete m_out;
}

LIns* NativeGenerator::generateOSRExit(size_t currentByteCodeIndex)
{
    m_out->insStore(LIR_sti, m_oneI, m_contextP, ExecutionContext::offsetofInOSRExit(), 1);

    bool isDone = false;
    unsigned writeCount = 0;
    bool* writeFlags = NULL;
    int maxStackPos = -1;
    for (int i = m_graph->basicBlockSize() - 1; i >= 0; i--) {
        ESBasicBlock* block = m_graph->basicBlock(i);
        if (block->instructionSize() > 0 && block->instruction(0)->targetIndex() <= currentByteCodeIndex) {
            int j = block->instructionSize() - 1;
            if (maxStackPos == -1) {
                for (; j >= 0; j--) {
                    if (block->instruction(j)->targetIndex() == currentByteCodeIndex) {
                        if (j == 0)
                            maxStackPos = 0;
                        else {
                            while (true) {
                                j--;
                                if (j < 0)
                                    break;
                                if (block->instruction(j)->targetIndex() != -1) {
                                    unsigned followPopCount = m_graph->getFollowPopCountOf(block->instruction(j)->targetIndex());
                                    maxStackPos = m_graph->getOperandStackPos(block->instruction(j)->targetIndex()) - followPopCount;
                                    writeFlags = (bool *) alloca(maxStackPos);
                                    memset(writeFlags, 0, maxStackPos * sizeof(bool));
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
            }

            if (maxStackPos > 0) {
                for (; j >= 0; j--) {
                    ESIR* esir = block->instruction(j);
                    if (esir->targetIndex() < 0) continue;
                    unsigned stackPos = m_graph->getOperandStackPos(esir->targetIndex());
                    if (!(stackPos > 0 && stackPos <= maxStackPos && !writeFlags[stackPos - 1])) continue;
                    Type type = m_graph->getOperandType(esir->targetIndex());
                    LIns* lIns = m_tmpToLInsMapping[esir->targetIndex()];
                    if (lIns) {
                        LIns* boxedLIns = boxESValue(lIns, type);
                        int bufOffset = (stackPos-1) * sizeof(ESValue);
                        LIns* stackBuf = m_out->insLoad(LIR_ldp, m_contextP, ExecutionContext::offsetofStackBuf(), 1, LOAD_NORMAL);
                        m_out->insStore(LIR_ste, boxedLIns, stackBuf, bufOffset, 1);
                        writeFlags[stackPos - 1] = true;
                        writeCount++;
                    }
                    if (writeCount == maxStackPos) {
                        LIns* maxStackPosLIns = m_out->insImmI(maxStackPos);
                        m_out->insStore(LIR_sti, maxStackPosLIns, m_contextP, ExecutionContext::offsetofStackPos(), 1);
                        isDone = true;
                        break;
                    }
                }
            } else {
                LIns* maxStackPosLIns = m_out->insImmI(maxStackPos);
                m_out->insStore(LIR_sti, maxStackPosLIns, m_contextP, ExecutionContext::offsetofStackPos(), 1);
                isDone = true;
            }
            if (isDone) break;
        }
    }

    LIns* bytecode = m_out->insImmI(currentByteCodeIndex);
    LIns* boxedIndex = boxESValue(bytecode, TypeInt32);
    return m_out->ins1(LIR_rete, boxedIndex);
}

nanojit::LIns* NativeGenerator::generateTypeCheck(LIns* in, Type type, size_t currentByteCodeIndex)
{
#ifdef ESCARGOT_64
    ASSERT(in->isQ());
#else
    ASSERT(in->isD());
#endif
#ifndef NDEBUG
    m_out->insComment(".= typecheck start =.");
#endif
    if (type.isBooleanType()) {
#ifdef ESCARGOT_64
        LIns* quadValue = in;
        LIns* maskedValue = m_out->ins2(LIR_orq, quadValue, m_booleanTagQ);
        LIns* maskedValue2 = m_out->ins2(LIR_subq, quadValue, m_booleanTagQ);
        LIns* checkIfBoolean = m_out->ins2(LIR_leuq, maskedValue2, m_out->insImmQ(1));
        LIns* jumpIfBoolean = m_out->insBranch(LIR_jt, checkIfBoolean, (LIns*)nullptr);
#ifndef NDEBUG
        if (ESVMInstance::currentInstance()->m_verboseJIT) {
            JIT_LOG(in, "Expected Boolean-typed value, but got this value");
            LIns* index = m_out->insImmI(currentByteCodeIndex);
            JIT_LOG(index, "currentByteCodeIndex = ");
        }
#endif
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out->ins0(LIR_label);
        jumpIfBoolean->setTarget(normalPath);
#else
        return nullptr;
#endif
    } else if (type.isInt32Type()) {
#ifdef ESCARGOT_64
        LIns* quadValue = in;
        LIns* maskedValue = m_out->ins2(LIR_andq, quadValue, m_intTagQ);
        LIns* checkIfInt = m_out->ins2(LIR_eqq, maskedValue, m_intTagQ);
        LIns* jumpIfInt = m_out->insBranch(LIR_jt, checkIfInt, (LIns*)nullptr);
#ifndef NDEBUG
        if (ESVMInstance::currentInstance()->m_verboseJIT) {
            JIT_LOG(in, "Expected Int-typed value, but got this value");
            LIns* index = m_out->insImmI(currentByteCodeIndex);
            JIT_LOG(index, "currentByteCodeIndex = ");
        }
#endif
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out->ins0(LIR_label);
        jumpIfInt->setTarget(normalPath);
#else
        return nullptr;
#endif
    } else if (type.isDoubleType()) {
#ifdef ESCARGOT_64
        LIns* result = m_out->insAlloc(sizeof(ESValue));
        LIns* quadValue = in;
        LIns* maskedValue = m_out->ins2(LIR_andq, quadValue, m_intTagQ);
        LIns* checkIfNotNumber = m_out->ins2(LIR_eqq, maskedValue, m_zeroQ);
        LIns* exitIfNotNumber = m_out->insBranch(LIR_jt, checkIfNotNumber, (LIns*)nullptr);
        m_out->insStore(LIR_ste, in, result, 0, 1);
        LIns* checkIfInt = m_out->ins2(LIR_eqq, maskedValue, m_intTagQ);
        LIns* jumpIfDouble = m_out->insBranch(LIR_jf, checkIfInt, (LIns*)nullptr);
        // in is int. convert int to double
        LIns* inAsInt = unboxESValue(in, Type(TypeInt32));
        // JIT_LOG(in, "int input");
        LIns* inAsDouble = m_out->ins1(LIR_i2d, inAsInt);

        LIns* doubleQuadUnboxedValue = m_out->ins1(LIR_dasq, inAsDouble);
        LIns* doubleBoxedValue = m_out->ins2(LIR_addq, doubleQuadUnboxedValue, m_doubleEncodeOffsetQ);
        LIns* doubleESValue = doubleBoxedValue;
        // JIT_LOG(doubleESValue, "out esvalue");
        m_out->insStore(LIR_ste, doubleESValue, result, 0, 1);
        // convert end. jump to normal path
        LIns* jumpToNormalPath = m_out->insBranch(LIR_j, nullptr, (LIns*)nullptr);
        LIns* exitPath = m_out->ins0(LIR_label);
        exitIfNotNumber->setTarget(exitPath);

#ifndef NDEBUG
        if (ESVMInstance::currentInstance()->m_verboseJIT) {
            JIT_LOG(in, "Expected Double-typed value, but got this value");
            LIns* index = m_out->insImmI(currentByteCodeIndex);
            JIT_LOG(index, "currentByteCodeIndex = ");
        }
#endif
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out->ins0(LIR_label);
        jumpIfDouble->setTarget(normalPath);
        jumpToNormalPath->setTarget(normalPath);
        return m_out->insLoad(LIR_lde, result, 0, 1, LOAD_NORMAL);
#else
        return nullptr;
#endif
    } else if (type.isObjectType() || type.isArrayObjectType() || type.isStringType() || type.isFunctionObjectType()) {
#ifdef ESCARGOT_64
        LIns* quadValue = in;
        LIns* maskedValue = m_out->ins2(LIR_andq, quadValue, m_tagMaskQ);
        LIns* checkIfNotTagged = m_out->ins2(LIR_eqq, maskedValue, m_zeroQ);
        LIns* jumpIfPointer = m_out->insBranch(LIR_jt, checkIfNotTagged, (LIns*)nullptr);
#ifndef NDEBUG
        if (ESVMInstance::currentInstance()->m_verboseJIT) {
            JIT_LOG(in, "Expected Pointer-typed value, but got this value");
            LIns* index = m_out->insImmI(currentByteCodeIndex);
            JIT_LOG(index, "currentByteCodeIndex = ");
        }
#endif
        generateOSRExit(currentByteCodeIndex);

        LIns* normalPath = m_out->ins0(LIR_label);
        jumpIfPointer->setTarget(normalPath);

        LIns* mask;
        if (type.isStringType())
            mask = m_out->insImmQ(ESPointer::Type::ESString);
        else if (type.isObjectType())
            mask = m_out->insImmQ(ESPointer::Type::ESObject);
        else if (type.isFunctionObjectType())
            mask = m_out->insImmQ(ESPointer::Type::ESFunctionObject);
        else if (type.isArrayObjectType())
            mask = m_out->insImmQ(ESPointer::Type::ESArrayObject);
        else
            return nullptr;
        LIns* typeOfESPtr = m_out->insLoad(LIR_lde, quadValue, ESPointer::offsetOfType(), 1, LOAD_NORMAL);
        LIns* esPointerMaskedValue = m_out->ins2(LIR_andq, typeOfESPtr, mask);
        LIns* checkIfFlagIdentical = m_out->ins2(LIR_eqq, esPointerMaskedValue, mask);
        LIns* jumpIfFlagIdentical = m_out->insBranch(LIR_jt, checkIfFlagIdentical, (LIns*)nullptr);
#ifndef NDEBUG
        if (ESVMInstance::currentInstance()->m_verboseJIT) {
            JIT_LOG(in, "Expected below-typed value, but got this value");
            JIT_LOG(in, type.getESIRTypeName());
            JIT_LOG(typeOfESPtr, "ESPointer Type : ");
            LIns* index = m_out->insImmI(currentByteCodeIndex);
            JIT_LOG(index, "currentByteCodeIndex = ");
        }
#endif
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath2 = m_out->ins0(LIR_label);
        jumpIfFlagIdentical->setTarget(normalPath2);
#else
        return nullptr;
#endif
    } else if (type.isPointerType()) {
#ifdef ESCARGOT_64
        LIns* quadValue = in;
        LIns* maskedValue = m_out->ins2(LIR_andq, quadValue, m_tagMaskQ);
        LIns* checkIfNotTagged = m_out->ins2(LIR_eqq, maskedValue, m_zeroQ);
        LIns* jumpIfPointer = m_out->insBranch(LIR_jt, checkIfNotTagged, (LIns*)nullptr);
#ifndef NDEBUG
        if (ESVMInstance::currentInstance()->m_verboseJIT) {
            JIT_LOG(in, "Expected Pointer-typed value, but got this value");
        }
#endif
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out->ins0(LIR_label);
        jumpIfPointer->setTarget(normalPath);
#else
        return nullptr;
#endif
    } else if (type.isUndefinedType()) {
#ifdef ESCARGOT_64
        LIns* quadValue = in;
        LIns* checkIfUndefined = m_out->ins2(LIR_eqq, quadValue, m_undefinedQ);
        LIns* jumpIfUndefined = m_out->insBranch(LIR_jt, checkIfUndefined, (LIns*)nullptr);
#ifndef NDEBUG
        if (ESVMInstance::currentInstance()->m_verboseJIT) {
            JIT_LOG(in, "Expected undefined value, but got this value");
        }
#endif
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out->ins0(LIR_label);
        jumpIfUndefined->setTarget(normalPath);
#else
        return nullptr;
#endif
    } else if (type.isNullType()) {
#ifdef ESCARGOT_64
        LIns* quadValue = in;
        LIns* checkIfNull = m_out->ins2(LIR_eqq, quadValue, m_nullQ);
        LIns* jumpIfNull = m_out->insBranch(LIR_jt, checkIfNull, (LIns*)nullptr);
#ifndef NDEBUG
        if (ESVMInstance::currentInstance()->m_verboseJIT) {
            JIT_LOG(in, "Expected null value, but got this value");
        }
#endif
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out->ins0(LIR_label);
        jumpIfNull->setTarget(normalPath);
#else
        return nullptr;
#endif
    } else if (type.isNumberType()) {
#ifdef ESCARGOT_64
        LIns* quadValue = in;
        LIns* maskedValue = m_out->ins2(LIR_andq, quadValue, m_intTagQ);
        LIns* checkIfNotNumber = m_out->ins2(LIR_eqq, maskedValue, m_zeroQ);
        LIns* jumpIfNumber = m_out->insBranch(LIR_jf, checkIfNotNumber, (LIns*)nullptr);
#ifndef NDEBUG
        if (ESVMInstance::currentInstance()->m_verboseJIT) {
            JIT_LOG(in, "Expected number value, but got this value");
            LIns* index = m_out->insImmI(currentByteCodeIndex);
            JIT_LOG(index, "currentByteCodeIndex = ");
        }
#endif
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out->ins0(LIR_label);
        jumpIfNumber->setTarget(normalPath);
#else
        return nullptr;
#endif
    } else {
        return nullptr;
    }
#ifndef NDEBUG
    m_out->insComment("'= typecheck ended ='");
#endif
    return in;
}

LIns* NativeGenerator::boxESValue(LIns* unboxedValue, Type type)
{
    LIns* boxedValueInDouble = nullptr;
    if (type.isBooleanType()) {
        ASSERT(unboxedValue->isI());
#ifdef ESCARGOT_64
        LIns* wideUnboxedValue = m_out->ins1(LIR_i2q, unboxedValue);
        LIns* boxedValue = m_out->ins2(LIR_orq, wideUnboxedValue, m_booleanTagQ);
        boxedValueInDouble = boxedValue;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isInt32Type()) {
        ASSERT(unboxedValue->isI());
#ifdef ESCARGOT_64
        LIns* wideUnboxedValue = m_out->ins1(LIR_i2q, unboxedValue);
        LIns* boxedValue = m_out->ins2(LIR_orq, wideUnboxedValue, m_intTagQ);
        boxedValueInDouble = boxedValue;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isDoubleType()) {
        ASSERT(unboxedValue->isD());
#ifdef ESCARGOT_64
        // LIns* args[] = { unboxedValue };
        // boxedValueInDouble = m_out->insCall(&workaroundForSavingDoubleCallInfo, args);
        LIns* result = m_out->insAlloc(sizeof(ESValue));

        LIns* intResult = m_out->ins1(LIR_d2i, unboxedValue);
        LIns* revertToDobule = m_out->ins1(LIR_i2d, intResult);

        // LIns* doubleQuadUnboxedValue = m_out->ins1(LIR_dasq, unboxedValue);
        // LIns* doubleBoxedValue = m_out->ins2(LIR_addq, doubleQuadUnboxedValue, m_doubleEncodeOffsetQ);
        // boxedValueInDouble = doubleBoxedValue;

        LIns* checkSame = m_out->ins2(LIR_eqd, revertToDobule, unboxedValue);
        LIns* jumpIfSame = m_out->insBranch(LIR_jt, checkSame, (LIns *)nullptr); // if int, jump

        LIns* doubleQuadUnboxedValue = m_out->ins1(LIR_dasq, unboxedValue);
        LIns* doubleBoxedValue = m_out->ins2(LIR_addq, doubleQuadUnboxedValue, m_doubleEncodeOffsetQ);
        // boxedValueInDouble = doubleBoxedValue;
        m_out->insStore(LIR_stq, doubleBoxedValue, result, 0, 1);
        LIns* gotoEnd = m_out->insBranch(LIR_j, nullptr, (LIns *)nullptr);

        LIns* convertToInt = m_out->ins0(LIR_label);
        jumpIfSame->setTarget(convertToInt);
        LIns* intESValue = boxESValue(intResult, Type(TypeInt32));
        m_out->insStore(LIR_stq, intESValue, result, 0, 1);

        LIns* end = m_out->ins0(LIR_label);
        gotoEnd->setTarget(end);
        boxedValueInDouble = m_out->insLoad(LIR_lde, result, 0, 1, LOAD_NORMAL);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isPointerType()) {
        ASSERT(unboxedValue->isP());
#ifdef ESCARGOT_64
        // "pointer" : a quad on 64-bit machines
        boxedValueInDouble = unboxedValue;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isUndefinedType() || type.isNullType()) {
#ifdef ESCARGOT_64
        ASSERT(unboxedValue->isQ());
        boxedValueInDouble = unboxedValue;
#else
        ASSERT(unboxedValue->isD());
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isNumberType()) {
        ASSERT(unboxedValue->isQ());
#ifdef ESCARGOT_64
        boxedValueInDouble = unboxedValue;
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else {
#ifndef NDEBUG
        std::cout << "Unsupported type in NativeGenerator::boxESValue() : ";
        type.dump(std::cout);
        std::cout << std::endl;
#endif
        RELEASE_ASSERT_NOT_REACHED();
    }
#ifdef ESCARGOT_64
    ASSERT(boxedValueInDouble->isQ());
#else
    ASSERT(boxedValueInDouble->isD());
#endif
    return boxedValueInDouble;
}

LIns* NativeGenerator::unboxESValue(LIns* boxedValue, Type type)
{
#ifdef ESCARGOT_64
    ASSERT(boxedValue->isQ());
#else
    ASSERT(boxedValue->isD());
#endif
    LIns* unboxedValue = nullptr;
    if (type.isBooleanType()) {
#ifdef ESCARGOT_64
        LIns* boxedValueInQuad = boxedValue;
        LIns* unboxedValueInQuad = m_out->ins2(LIR_andq, boxedValueInQuad, m_booleanTagComplementQ);
        unboxedValue = m_out->ins1(LIR_q2i, unboxedValueInQuad);
        ASSERT(unboxedValue->isI());
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isInt32Type()) {
#ifdef ESCARGOT_64
        LIns* boxedValueInQuad = boxedValue;
        LIns* unboxedValueInQuad = m_out->ins2(LIR_andq, boxedValueInQuad, m_intTagComplementQ);
        unboxedValue = m_out->ins1(LIR_q2i, unboxedValueInQuad);
        ASSERT(unboxedValue->isI());
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isDoubleType()) {
#ifdef ESCARGOT_64
        LIns* boxedValueInQuad = boxedValue;
        LIns* doubleValue = m_out->ins2(LIR_subq, boxedValueInQuad, m_doubleEncodeOffsetQ);
        unboxedValue = m_out->ins1(LIR_qasd, doubleValue);
        ASSERT(unboxedValue->isD());
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isPointerType()) {
#ifdef ESCARGOT_64
        // "pointer" : a quad on 64-bit machines
        unboxedValue = boxedValue;
        ASSERT(unboxedValue->isP());
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isUndefinedType() || type.isNullType()) {
#ifdef ESCARGOT_64
        unboxedValue = boxedValue;
        ASSERT(unboxedValue->isQ());
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else if (type.isNumberType()) {
#ifdef ESCARGOT_64
        unboxedValue = boxedValue;
        ASSERT(unboxedValue->isQ());
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    } else {
#ifndef NDEBUG
        std::cout << "Unsupported type in NativeGenerator::unboxESValue() : ";
        type.dump(std::cout);
        std::cout << std::endl;
#endif
        RELEASE_ASSERT_NOT_REACHED();
    }
    return unboxedValue;
}

LIns* NativeGenerator::getDoubleDynamic(LIns* in, Type type)
{
    LIns* ret;
    if (type.isInt32Type()) {
        ASSERT(in->isI());
        ret = m_out->ins1(LIR_i2d, in);
    } else if (type.isDoubleType()) {
        ASSERT(in->isD());
        ret = in; // do nothing
    } else {
        ASSERT(type.isNumberType());
#ifdef ESCARGOT_64
        ASSERT(in->isQ());
        LIns* phi = m_out->insAlloc(sizeof(double));
        LIns* maskedValue = m_out->ins2(LIR_andq, in, m_intTagQ);

        LIns* checkIfInt = m_out->ins2(LIR_eqq, maskedValue, m_intTagQ);
        LIns* jumpIfDouble = m_out->insBranch(LIR_jf, checkIfInt, (LIns*)nullptr);

        LIns* intPath = m_out->ins0(LIR_label);
        LIns* untaggedIntValue = m_out->ins2(LIR_subq, in, m_intTagQ);
        LIns* doubleValue = m_out->ins1(LIR_q2d, untaggedIntValue);
        m_out->insStore(LIR_std, doubleValue, phi, 0, 1);

        LIns* doublePath = m_out->ins0(LIR_label);
        jumpIfDouble->setTarget(doublePath);
        LIns* untaggedDoubleValue = m_out->ins2(LIR_subq, in, m_doubleEncodeOffsetQ);
        LIns* doubleValue2 = m_out->ins1(LIR_qasd, untaggedDoubleValue);
        m_out->insStore(LIR_std, doubleValue2, phi, 0, 1);

        ret = m_out->insLoad(LIR_ldd, phi, 0, 1, LOAD_NORMAL);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    }

    ASSERT(ret->isD());
    return ret;
}

LIns* NativeGenerator::getInt32Dynamic(LIns* in, Type type)
{
    LIns* ret;
    if (type.isDoubleType()) {
        ASSERT(in->isD());
        ret = m_out->ins1(LIR_d2i, in);
    } else if (type.isInt32Type()) {
        ASSERT(in->isI());
        ret = in; // do nothing;
    } else {
        ASSERT(in->isQ());
        ASSERT(type.isNumberType());
#ifdef ESCARGOT_64
        ASSERT(in->isQ());
        LIns* phi = m_out->insAlloc(sizeof(int));
        LIns* maskedValue = m_out->ins2(LIR_andq, in, m_intTagQ);

        LIns* checkIfInt = m_out->ins2(LIR_eqq, maskedValue, m_intTagQ);
        LIns* jumpIfDouble = m_out->insBranch(LIR_jf, checkIfInt, (LIns*)nullptr);

        LIns* intPath = m_out->ins0(LIR_label);
        LIns* untaggedValue = m_out->ins2(LIR_subq, in, m_intTagQ);
        LIns* intValue = m_out->ins1(LIR_q2i, untaggedValue);
        m_out->insStore(LIR_sti, in, phi, 0, 1);

        LIns* doublePath = m_out->ins0(LIR_label);
        jumpIfDouble->setTarget(doublePath);
        LIns* untaggedDoubleValue = m_out->ins2(LIR_subq, in, m_doubleEncodeOffsetQ);
        LIns* intValue2 = m_out->ins1(LIR_q2i, untaggedDoubleValue);
        m_out->insStore(LIR_sti, intValue2, phi, 0, 1);

        ret = m_out->insLoad(LIR_ldi, phi, 0, 1, LOAD_NORMAL);
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
    }
    ASSERT(ret->isI());
    return ret;
}

LIns* NativeGenerator::nanojitCodegen(ESIR* ir)
{
    switch (ir->opcode()) {
#ifndef NDEBUG
#define INIT_ESIR(opcode) \
        opcode##IR* ir##opcode = static_cast<opcode##IR*>(ir); \
        m_out->insComment("# # # # # # # # Opcode " #opcode " # # # # # # # # #");
#else
#define INIT_ESIR(opcode) \
        opcode##IR* ir##opcode = static_cast<opcode##IR*>(ir);
#endif
    case ESIR::Opcode::ConstantESValue:
        {
            INIT_ESIR(ConstantESValue);
#ifdef ESCARGOT_64
            return m_out->insImmQ(irConstantESValue->value());
#else
            return m_out->insImmD(irConstantESValue->value());
#endif
        }
    case ESIR::Opcode::ConstantInt:
        {
            INIT_ESIR(ConstantInt);
            return  m_out->insImmI(irConstantInt->value());
        }
    case ESIR::Opcode::ConstantDouble:
        {
            INIT_ESIR(ConstantDouble);
            return m_out->insImmD(irConstantDouble->value());
        }
    case ESIR::Opcode::ConstantBoolean:
        {
            INIT_ESIR(ConstantBoolean);
            return m_out->insImmI(irConstantBoolean->value());
        }
    case ESIR::Opcode::ConstantString:
        {
            INIT_ESIR(ConstantString);
            return m_out->insImmP(irConstantString->value());
        }
    case ESIR::Opcode::ConstantPointer:
        {
            INIT_ESIR(ConstantPointer);
            return m_out->insImmP(irConstantPointer->value());
        }
#define INIT_BINARY_ESIR(opcode) \
            Type leftType = m_graph->getOperandType(ir##opcode->leftIndex()); \
            Type rightType = m_graph->getOperandType(ir##opcode->rightIndex()); \
            LIns* left = getTmpMapping(ir##opcode->leftIndex()); \
            LIns* right = getTmpMapping(ir##opcode->rightIndex());
#define CALL_BINARY_ESIR(opcode, callInfo) \
            LIns* boxedLeft = boxESValue(left, leftType); \
            LIns* boxedRight = boxESValue(right, rightType); \
            LIns* args[] = {boxedRight, boxedLeft}; \
            LIns* boxedResult = m_out->insCall(&callInfo, args); \
            Type expectedType = m_graph->getOperandType(ir##opcode->m_targetIndex); \
            LIns* unboxedResult = unboxESValue(boxedResult, expectedType);
#define INIT_UNARY_ESIR(opcode) \
            Type valueType = m_graph->getOperandType(ir##opcode->sourceIndex()); \
            LIns* value = getTmpMapping(ir##opcode->sourceIndex());
#define CALL_UNARY_ESIR(opcode, callInfo) \
            LIns* boxedValue = boxESValue(value, valueType); \
            LIns* args[] = {boxedValue}; \
            LIns* boxedResult = m_out->insCall(&callInfo, args); \
            Type expectedType = m_graph->getOperandType(ir##opcode->m_targetIndex); \
            LIns* unboxedResult = unboxESValue(boxedResult, expectedType);
    case ESIR::Opcode::Int32Plus:
        {
            INIT_ESIR(Int32Plus);
            INIT_BINARY_ESIR(Int32Plus);
            ASSERT(leftType.isInt32Type() && rightType.isInt32Type());
            ASSERT(left->isI() && right->isI());
            LIns* int32Result = m_out->insBranchJov(LIR_addjovi, left, right, (LIns*)nullptr);
            LIns* jumpAlways = m_out->insBranch(LIR_j, nullptr, (LIns*)nullptr);
            LIns* labelOverflow = m_out->ins0(LIR_label);
            int32Result->setTarget(labelOverflow);

            JIT_LOG(int32Result, "Int32Plus : Result is not int32");
            generateOSRExit(irInt32Plus->targetIndex());

            LIns* labelNoOverflow = m_out->ins0(LIR_label);
            jumpAlways->setTarget(labelNoOverflow);
            return int32Result;
        }
    case ESIR::Opcode::DoublePlus:
        {
            INIT_ESIR(DoublePlus);
            INIT_BINARY_ESIR(DoublePlus);
            left = getDoubleDynamic(left, leftType);
            right = getDoubleDynamic(right, rightType);
            return m_out->ins2(LIR_addd, left, right);
        }
    case ESIR::Opcode::StringPlus:
        {
            INIT_ESIR(StringPlus);
            INIT_BINARY_ESIR(StringPlus);

            if (!leftType.isStringType()) {
                LIns* boxedLeft = boxESValue(left, leftType);
                LIns* args[] = {boxedLeft};
                left = m_out->insCall(&generateToStringCallInfo, args);
            }

            if (!rightType.isStringType()) {
                LIns* boxedRight = boxESValue(right, rightType);
                LIns* args[] = {boxedRight};
                right = m_out->insCall(&generateToStringCallInfo, args);
            }

            LIns* args[] = {right, left};
            return m_out->insCall(&concatTwoStringsCallInfo, args);
        }
    case ESIR::Opcode::GenericPlus:
        {
            INIT_ESIR(GenericPlus);
            INIT_BINARY_ESIR(GenericPlus);

            LIns* boxedLeft = boxESValue(left, leftType);
            LIns* boxedRight = boxESValue(right, rightType);
            LIns* args[] = {boxedRight, boxedLeft};
            return m_out->insCall(&plusOpCallInfo, args);
        }
    case ESIR::Opcode::Minus:
        {
            INIT_ESIR(Minus);
            INIT_BINARY_ESIR(Minus);

            if (leftType.isInt32Type() && rightType.isInt32Type()) {
                // TODO implement overflow, underflow
                return m_out->ins2(LIR_subi, left, right);
            } else if (leftType.isNumberType() && rightType.isNumberType()) {
                left = getDoubleDynamic(left, leftType);
                right = getDoubleDynamic(right, rightType);
                return m_out->ins2(LIR_subd, left, right);
            }
            else {
                LIns* boxedLeft = boxESValue(left, TypeInt32);
                LIns* boxedRight = boxESValue(right, TypeInt32);
                LIns* args[] = {boxedRight, boxedLeft};
                LIns* boxedResult = m_out->insCall(&minusOpCallInfo, args);
                LIns* unboxedResult = unboxESValue(boxedResult, TypeInt32);
                return unboxedResult;
            }
        }
    case ESIR::Opcode::Int32Multiply:
        {
            INIT_ESIR(Int32Multiply);
            INIT_BINARY_ESIR(Int32Multiply);
#if 1
            ASSERT(left->isI() && right->isI());
            /*
            LIns* int32Result = m_out->insBranchJov(LIR_muljovi, left, right, (LIns*)nullptr);
            LIns* jumpAlways = m_out->insBranch(LIR_j, nullptr, (LIns*)nullptr);
            LIns* int32Result = m_out->insBranchJov(LIR_muljovi, left, right, nullptr);
            LIns* jumpAlways = m_out->insBranch(LIR_j, nullptr, nullptr);
            LIns* labelOverflow = m_out->ins0(LIR_label);
            int32Result->setTarget(labelOverflow);

            JIT_LOG(int32Result, "Int32Multiply : Result is not int32");
            generateOSRExit(irInt32Multiply->targetIndex());

            LIns* labelNoOverflow = m_out->ins0(LIR_label);
            jumpAlways->setTarget(labelNoOverflow);
            return int32Result;
            */
            LIns* doubleA = m_out->ins1(LIR_i2d, left);
            LIns* doubleB = m_out->ins1(LIR_i2d, right);

            return m_out->ins2(LIR_muld, doubleA, doubleB);
#else
            return m_out->ins2(LIR_muli, left, right);
#endif
        }
    case ESIR::Opcode::DoubleMultiply:
        {
            INIT_ESIR(DoubleMultiply);
            INIT_BINARY_ESIR(DoubleMultiply);
            left = getDoubleDynamic(left, leftType);
            right = getDoubleDynamic(right, rightType);
            return m_out->ins2(LIR_muld, left, right);
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
            left = getDoubleDynamic(left, leftType);
            right = getDoubleDynamic(right, rightType);
            return m_out->ins2(LIR_divd, left, right);
        }
    case ESIR::Opcode::GenericDivision:
        {
            // FIXME
            return nullptr;
        }
    case ESIR::Opcode::Int32Mod:
        {
            INIT_ESIR(Int32Mod);
            INIT_BINARY_ESIR(Int32Mod);
            ASSERT(left->isI() && right->isI());
            ASSERT(leftType.isInt32Type() && rightType.isInt32Type());
            LIns* res = m_out->ins2(LIR_divi, left, right);
            res = m_out->ins2(LIR_muli, res, right);
            res = m_out->ins2(LIR_subi, left, res);
            return res; // e_out.ins2(LIR_modi, left, right);
        }
    case ESIR::Opcode::DoubleMod:
        {
            INIT_ESIR(DoubleMod);
            INIT_BINARY_ESIR(DoubleMod);
            left = getDoubleDynamic(left, leftType);
            right = getDoubleDynamic(right, rightType);

            // FIXME: consider minus left
            /*
            left = m_out->ins1(LIR_absd, left);
            right = m_out->ins1(LIR_absd, right);
            */
            LIns* res = m_out->ins2(LIR_divd, left, right);
            res = m_out->ins1(LIR_d2i, res);
            res = m_out->ins1(LIR_i2d, res);
            res = m_out->ins2(LIR_muld, res, right);
            res = m_out->ins2(LIR_subd, left, res);
            return res;
        }
    case ESIR::Opcode::GenericMod:
        {
            // FIXME
            return nullptr;
        }
    case ESIR::Opcode::BitwiseAnd:
        {
            INIT_ESIR(BitwiseAnd);
            INIT_BINARY_ESIR(BitwiseAnd);
            if (leftType.isInt32Type() && rightType.isInt32Type())
                return m_out->ins2(LIR_andi, left, right);
            else {
                // TODO : call function to handle non-number cases
                return nullptr;
            }
        }
    case ESIR::Opcode::BitwiseOr:
        {
            INIT_ESIR(BitwiseOr);
            INIT_BINARY_ESIR(BitwiseOr);
            if (leftType.isNumberType() && rightType.isNumberType()) {
                left = getInt32Dynamic(left, leftType);
                right = getInt32Dynamic(right, rightType);
                return m_out->ins2(LIR_ori, left, right);
            } else {
                CALL_BINARY_ESIR(BitwiseOr, bitwiseOrOpCallInfo);
                return unboxedResult;
            }
        }
    case ESIR::Opcode::BitwiseXor:
        {
            INIT_ESIR(BitwiseXor);
            INIT_BINARY_ESIR(BitwiseXor);
            if (leftType.isNumberType() && rightType.isNumberType()) {
                left = getInt32Dynamic(left, leftType);
                right = getInt32Dynamic(right, rightType);
                return m_out->ins2(LIR_xori, left, right);
            } else {
                CALL_BINARY_ESIR(BitwiseXor, bitwiseXorOpCallInfo);
                return unboxedResult;
            }
        }
    case ESIR::Opcode::Equal:
        {
            INIT_ESIR(Equal);
            INIT_BINARY_ESIR(Equal);

            bool isLeftUndefinedOrNull = leftType.isNullType() || leftType.isUndefinedType();
            bool isRightUndefinedOrNull = rightType.isNullType() || rightType.isUndefinedType();
            if (leftType.isInt32Type() && rightType.isInt32Type())
                return m_out->ins2(LIR_eqi, left, right);
            else if (isLeftUndefinedOrNull && isRightUndefinedOrNull)
                return m_true;
            else if (isLeftUndefinedOrNull || isRightUndefinedOrNull)
                return m_false;
            else {
                CALL_BINARY_ESIR(Equal, equalOpCallInfo);
                return unboxedResult;
            }
        }
    case ESIR::Opcode::NotEqual:
        {
            INIT_ESIR(NotEqual);
            INIT_BINARY_ESIR(NotEqual);

            LIns* ret;
            bool isLeftUndefinedOrNull = leftType.isNullType() || leftType.isUndefinedType();
            bool isRightUndefinedOrNull = rightType.isNullType() || rightType.isUndefinedType();
            if (leftType.isInt32Type() && rightType.isInt32Type())
                ret = m_out->ins2(LIR_eqi, left, right);
            else if (isLeftUndefinedOrNull && isRightUndefinedOrNull)
                ret = m_true;
            else if (isLeftUndefinedOrNull || isRightUndefinedOrNull)
                ret = m_false;
            else {
                CALL_BINARY_ESIR(NotEqual, equalOpCallInfo);
                ret = unboxedResult;
            }

            return m_out->ins2(LIR_xori, ret, m_oneI);
        }
    case ESIR::Opcode::StrictEqual:
        {
            INIT_ESIR(StrictEqual);
            INIT_BINARY_ESIR(StrictEqual);

            if (leftType != rightType) {
                return m_false;
            } else {
                if (leftType.isInt32Type() || leftType.isBooleanType()) {
                    return m_out->ins2(LIR_eqi, left, right);
                } else {
                    CALL_BINARY_ESIR(StrictEqual, strictEqualOpCallInfo);
                    return unboxedResult;
                }
            }
        }
    case ESIR::Opcode::NotStrictEqual:
        {
            INIT_ESIR(NotStrictEqual);
            INIT_BINARY_ESIR(NotStrictEqual);

            LIns* ret;
            if (leftType != rightType) {
                ret = m_false;
            } else {
                if (leftType.isInt32Type() || leftType.isBooleanType()) {
                    ret = m_out->ins2(LIR_eqi, left, right);
                } else {
                    CALL_BINARY_ESIR(NotStrictEqual, strictEqualOpCallInfo);
                    ret = unboxedResult;
                }
            }

            return m_out->ins2(LIR_xori, ret, m_oneI);
        }
    case ESIR::Opcode::GreaterThan:
        {
            INIT_ESIR(GreaterThan);
            INIT_BINARY_ESIR(GreaterThan);
            if (leftType.isNumberType() && rightType.isNumberType()) {
                if (leftType.isInt32Type() && rightType.isInt32Type())
                    return m_out->ins2(LIR_gti, left, right);
                else {
                    left = getDoubleDynamic(left, leftType);
                    right = getDoubleDynamic(right, rightType);
                    return m_out->ins2(LIR_gtd, left, right);
                }
            }
            else
                return nullptr;
        }
    case ESIR::Opcode::GreaterThanOrEqual:
        {
            INIT_ESIR(GreaterThanOrEqual);
            INIT_BINARY_ESIR(GreaterThanOrEqual);
            if (leftType.isInt32Type() && rightType.isInt32Type())
                return m_out->ins2(LIR_gei, left, right);
            else if (leftType.isNumberType() && rightType.isNumberType()) {
                left = getDoubleDynamic(left, leftType);
                right = getDoubleDynamic(right, rightType);
                return m_out->ins2(LIR_ged, left, right);
            } else
                return nullptr;
        }
    case ESIR::Opcode::LessThan:
        {
            INIT_ESIR(LessThan);
            INIT_BINARY_ESIR(LessThan);
            if (leftType.isInt32Type() && rightType.isInt32Type())
                return m_out->ins2(LIR_lti, left, right);
            else if (leftType.isNumberType() && rightType.isNumberType()) {
                left = getDoubleDynamic(left, leftType);
                right = getDoubleDynamic(right, rightType);
                return m_out->ins2(LIR_ltd, left, right);
            }
            else if (leftType.isUndefinedType() || rightType.isUndefinedType())
                return m_false;
            else {
                CALL_BINARY_ESIR(LessThan, lessThanOpCallInfo);
                return unboxedResult;
            }
        }
    case ESIR::Opcode::LessThanOrEqual:
        {
            INIT_ESIR(LessThanOrEqual);
            INIT_BINARY_ESIR(LessThanOrEqual);
            if (leftType.isInt32Type() && rightType.isInt32Type())
                return m_out->ins2(LIR_lei, left, right);
            if (leftType.isDoubleType() && rightType.isDoubleType())
                return m_out->ins2(LIR_led, left, right);
            else
                return nullptr;
        }
    case ESIR::Opcode::LeftShift:
        {
            INIT_ESIR(LeftShift);
            INIT_BINARY_ESIR(LeftShift);
            if (leftType.isInt32Type() && rightType.isInt32Type())
                return m_out->ins2(LIR_lshi, left, right);
            else {
                CALL_BINARY_ESIR(LeftShift, leftShiftOpCallInfo);
                return unboxedResult;
            }
        }
    case ESIR::Opcode::SignedRightShift:
        {
            INIT_ESIR(SignedRightShift);
            INIT_BINARY_ESIR(SignedRightShift);
            if (leftType.isNumberType() && rightType.isNumberType()) {
                left = getInt32Dynamic(left, leftType);
                right = getInt32Dynamic(right, rightType);
                return m_out->ins2(LIR_rshi, left, right);
            }
            else {
                CALL_BINARY_ESIR(SignedRightShift, signedRightShiftOpCallInfo);
                return unboxedResult;
            }
        }
    case ESIR::Opcode::UnsignedRightShift:
        {
            INIT_ESIR(UnsignedRightShift);
            INIT_BINARY_ESIR(UnsignedRightShift);
            if (leftType.isNumberType() && rightType.isNumberType()) {
                left = getInt32Dynamic(left, leftType);
                right = getInt32Dynamic(right, rightType);
                return m_out->ins2(LIR_rshui, left, right);
            }
            else {
                CALL_BINARY_ESIR(UnsignedRightShift, unsignedRightShiftOpCallInfo);
                return unboxedResult;
            }
        }
    case ESIR::Opcode::Jump:
        {
            INIT_ESIR(Jump);
            return m_out->insBranch(LIR_jt, m_true, irJump->targetBlock());
        }
    case ESIR::Opcode::Throw:
        {
            INIT_ESIR(Throw);
            LIns* source = getTmpMapping(irThrow->sourceIndex());
            LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irThrow->sourceIndex()));
            LIns* args[] = {boxedSource};
            return m_out->insCall(&throwCallInfo, args);
        }
    case ESIR::Opcode::Branch:
        {
            INIT_ESIR(Branch);
            // TODO implement double type : can't deal with NaN using eqd
            LIns* condition = getTmpMapping(irBranch->operandIndex());
            Type conditionType = m_graph->getOperandType(irBranch->operandIndex());
            LIns* compare = nullptr;
            if (conditionType.isInt32Type() || conditionType.isBooleanType()) {
                compare = m_out->ins2(LIR_eqi, condition, m_zeroI);
            } else if (conditionType.isUndefinedType() || conditionType.isNullType()) {
                compare = m_oneI;
            } else {
                LIns* boxedValue = boxESValue(condition, conditionType);
                LIns* args[] = {boxedValue};
                LIns* toBoolean = m_out->insCall(&toBooleanCallInfo, args);
                compare = m_out->ins2(LIR_eqi, toBoolean, m_zeroI);
            }

            LIns* jumpTrue = m_out->insBranch(LIR_jf, compare, irBranch->trueBlock());
            LIns* jumpFalse = m_out->insBranch(LIR_j, nullptr, irBranch->falseBlock());
            return jumpFalse;
        }
    case ESIR::Opcode::CreateFunction:
        {
            INIT_ESIR(CreateFunction);
            LIns* bytecode = m_out->insImmP(irCreateFunction->originalByteCode());
            LIns* args[] = {bytecode, m_contextP};
            return m_out->insCall(&createFunctionCallInfo, args);;
        }
    case ESIR::Opcode::CallJS:
        {
            INIT_ESIR(CallJS);
            LIns* callee = getTmpMapping(irCallJS->calleeIndex());
            LIns* boxedCallee = boxESValue(callee,  m_graph->getOperandType(irCallJS->calleeIndex()));
            LIns* arguments;
            if (irCallJS->argumentCount() > 0) {
                arguments = m_out->insAlloc(irCallJS->argumentCount() * sizeof(ESValue));
            } else {
                arguments = m_out->insImmP(0);
            }
            LIns* argumentCount = m_out->insImmI(irCallJS->argumentCount());
            if (irCallJS->receiverIndex() == -1) {
                for (size_t i=0; i<irCallJS->argumentCount(); i++) {
                    LIns* argument = getTmpMapping(irCallJS->argumentIndex(i));
                    LIns* boxedArgument = boxESValue(argument, m_graph->getOperandType(irCallJS->argumentIndex(i)));
                    m_out->insStore(LIR_ste, boxedArgument, arguments, i * sizeof(ESValue), 1);
                }
                LIns* args[] = {m_false, argumentCount, arguments, boxedCallee, m_instanceP};
                LIns* boxedResult = m_out->insCall(&esFunctionObjectCallCallInfo, args);
                return boxedResult;
            } else {
                LIns* receiver = getTmpMapping(irCallJS->receiverIndex());
                LIns* boxedReceiver = boxESValue(receiver,  m_graph->getOperandType(irCallJS->receiverIndex()));
                for (size_t i=0; i<irCallJS->argumentCount(); i++) {
                    LIns* argument = getTmpMapping(irCallJS->argumentIndex(i));
                    LIns* boxedArgument = boxESValue(argument, m_graph->getOperandType(irCallJS->argumentIndex(i)));
                    m_out->insStore(LIR_ste, boxedArgument, arguments, i * sizeof(ESValue), 1);
                }
                LIns* args[] = {m_false, argumentCount, arguments, boxedReceiver, boxedCallee, m_instanceP};
                LIns* boxedResult = m_out->insCall(&esFunctionObjectCallWithReceiverCallInfo, args);
                return boxedResult;
            }
        }
    case ESIR::Opcode::CallNewJS:
        {
            INIT_ESIR(CallNewJS);
            LIns* callee = getTmpMapping(irCallNewJS->calleeIndex());
            LIns* boxedCallee = boxESValue(callee,  m_graph->getOperandType(irCallNewJS->calleeIndex()));
            LIns* arguments;
            if (irCallNewJS->argumentCount() > 0) {
                arguments = m_out->insAlloc(irCallNewJS->argumentCount() * sizeof(ESValue));
            } else {
                arguments = m_out->insImmP(0);
            }
            LIns* argumentCount = m_out->insImmI(irCallNewJS->argumentCount());
            for (size_t i=0; i<irCallNewJS->argumentCount(); i++) {
                LIns* argument = getTmpMapping(irCallNewJS->argumentIndex(i));
                LIns* boxedArgument = boxESValue(argument, m_graph->getOperandType(irCallNewJS->argumentIndex(i)));
                m_out->insStore(LIR_ste, boxedArgument, arguments, i * sizeof(ESValue), 1);
            }
            LIns* args[] = {argumentCount, arguments, boxedCallee, m_globalObjectP, m_instanceP};
            LIns* boxedResult = m_out->insCall(&newOpCallInfo, args);
            return boxedResult;
        }
    case ESIR::Opcode::CallEval:
        {
            INIT_ESIR(CallEval);
            LIns* arguments;
            if (irCallEval->argumentCount() > 0) {
                arguments = m_out->insAlloc(irCallEval->argumentCount() * sizeof(ESValue));
            } else {
                arguments = m_out->insImmP(0);
            }
            LIns* argumentCount = m_out->insImmI(irCallEval->argumentCount());
            for (size_t i = 0; i < irCallEval->argumentCount(); i++) {
                LIns* argument = getTmpMapping(irCallEval->argumentIndex(i));
                LIns* boxedArgument = boxESValue(argument, m_graph->getOperandType(irCallEval->argumentIndex(i)));
                m_out->insStore(LIR_ste, boxedArgument, arguments, i * sizeof(ESValue), 1);
            }
            LIns* args[] = {arguments, argumentCount, m_contextP, m_instanceP};
            LIns* boxedResult = m_out->insCall(&evalCallCallInfo, args);
            return boxedResult;
        }
    case ESIR::Opcode::Return:
        {
            LIns* undefined = m_undefinedQ;
            return m_out->ins1(LIR_rete, undefined);
        }
    case ESIR::Opcode::ReturnWithValue:
        {
            INIT_ESIR(ReturnWithValue);
            LIns* returnValue = getTmpMapping(irReturnWithValue->returnIndex());
            LIns* returnESValue = boxESValue(returnValue, m_graph->getOperandType(irReturnWithValue->returnIndex()));
            // JIT_LOG(returnESValue, "Returning this value");
            return m_out->ins1(LIR_rete, returnESValue);
        }
    case ESIR::Opcode::Move:
        {
            INIT_ESIR(Move);
            return getTmpMapping(irMove->sourceIndex());
        }
    case ESIR::Opcode::GetEnumerablObjectData:
        {
            INIT_ESIR(GetEnumerablObjectData);
            LIns* enumerableObjectData = getTmpMapping(irGetEnumerablObjectData->sourceIndex());
            LIns* boxedEnumerableObjectData = boxESValue(enumerableObjectData, TypeObject);
            LIns* args[] = {boxedEnumerableObjectData};
            LIns* data = m_out->insCall(&getEnumerablObjectDataCallInfo, args);
            irGetEnumerablObjectData->getJumpIR()->targetBlock()->addInsToExtendLife(data);
            return data;
        }
    case ESIR::Opcode::CheckIfKeyIsLast:
        {
            INIT_ESIR(CheckIfKeyIsLast);
            LIns* data = getTmpMapping(irCheckIfKeyIsLast->sourceIndex());
            LIns* args[] = {data};
            LIns* keySize = m_out->insCall(&keySizeCallInfo, args);
            LIns* index = m_out->insLoad(LIR_ldi, data, offsetof(EnumerateObjectData, m_idx), 1, LOAD_NORMAL);
            LIns* isLastKey = m_out->ins2(LIR_eqi, index, keySize);
            return isLastKey;
        }
    case ESIR::Opcode::GetEnumerateKey:
        {
            INIT_ESIR(GetEnumerateKey);
            LIns* data = getTmpMapping(irGetEnumerateKey->sourceIndex());
            LIns* args[] = {data};
            LIns* index = m_out->insLoad(LIR_ldi, data, offsetof(EnumerateObjectData, m_idx), 1, LOAD_NORMAL);
            LIns* addOneToIndex = m_out->ins2(LIR_addi, index, m_oneI);
            LIns* storeToIndex = m_out->insStore(LIR_sti, addOneToIndex, data, offsetof(EnumerateObjectData, m_idx), 1);
            LIns* key = m_out->insCall(&getEnumerationKeyCallInfo, args);
            return key;
        }
    case ESIR::Opcode::GetThis:
        {
            INIT_ESIR(GetThis);

#ifdef ESCARGOT_64
            LIns* m_cachedThisValue = m_out->insLoad(LIR_lde, m_thisValueP, 0, 1, LOAD_NORMAL);
            LIns* checkIfThisValueisEmpty = m_out->ins2(LIR_eqq, m_cachedThisValue, m_emptyQ);
#else
            RELEASE_ASSERT_NOT_REACHED();
#endif
            LIns* jumpIfThisValueisNotEmpty = m_out->insBranch(LIR_jf, checkIfThisValueisEmpty, (LIns*)nullptr);
            LIns* args[] = {m_contextP};
            LIns* resolvedThisValue = m_out->insCall(&contextResolveThisBindingCallInfo, args);
            m_out->insStore(LIR_ste, resolvedThisValue, m_thisValueP, 0 , 1);

            LIns* thisValueIsValid = m_out->ins0(LIR_label);
            jumpIfThisValueisNotEmpty->setTarget(thisValueIsValid);

            m_out->ins0(LIR_label);
            return m_out->insLoad(LIR_lde, m_thisValueP, 0, 1, LOAD_NORMAL);
        }
    case ESIR::Opcode::GetArgument:
        {
            INIT_ESIR(GetArgument);
            LIns* argument = m_out->insLoad(LIR_lde, m_cachedDeclarativeEnvironmentRecordESValueP, irGetArgument->argumentIndex() * sizeof(ESValue), 1, LOAD_NORMAL);
            // JIT_LOG(argument, "Read this argument");
            return argument;
        }
    case ESIR::Opcode::GetVar:
        {
            INIT_ESIR(GetVar);
            if (irGetVar->varUpIndex() == 0 && !irGetVar->needsActivation()) {
                return m_out->insLoad(LIR_lde, m_cachedDeclarativeEnvironmentRecordESValueP, irGetVar->varIndex() * sizeof(ESValue), 1, LOAD_NORMAL);
            } else {
                // inline ESValueInDouble getByIndexWithActivationOp(ExecutionContext* ec, int32_t upCount, int32_t index)
                // LIns* args[] = {m_out->insImmI(irGetVar->varIndex()), m_out->insImmI(irGetVar->varUpIndex()), m_contextP};
                // return m_out->insCall(&getByIndexWithActivationOpCallInfo, args);

                LIns* ecIns = m_contextP;

                // TODO
                // if with statement are implemented, we should load re-implement loading env loading source
                LIns* envIns = m_out->insLoad(LIR_ldp, ecIns, ExecutionContext::offsetOfEnvironment(), 1, LOAD_NORMAL);

                for (int i = 0; i < irGetVar->varUpIndex(); i ++) {
                    envIns = m_out->insLoad(LIR_ldp, envIns, LexicalEnvironment::offsetofOuterEnvironment(), 1, LOAD_NORMAL);
                }

                LIns* record = m_out->insLoad(LIR_ldp, envIns, LexicalEnvironment::offsetofRecord(), 1, LOAD_NORMAL);
                LIns* vectorDataPointer = m_out->insLoad(LIR_ldp, record, (DeclarativeEnvironmentRecord::offsetofActivationData() + ESIdentifierVector::offsetofData()), 1, LOAD_NORMAL);
                return m_out->insLoad(LIR_lde, vectorDataPointer, sizeof(ESIdentifierVectorStdItem) * irGetVar->varIndex() + sizeof(InternalAtomicString), 1, LOAD_NORMAL);
            }
        }
    case ESIR::Opcode::SetVar:
        {
            INIT_ESIR(SetVar);
            LIns* source = getTmpMapping(irSetVar->sourceIndex());
            LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irSetVar->sourceIndex()));
            if (irSetVar->upVarIndex() == 0 && !irSetVar->needsActivation()) {
                m_out->insStore(LIR_ste, boxedSource, m_cachedDeclarativeEnvironmentRecordESValueP, irSetVar->localVarIndex() * sizeof(ESValue), 1);
            } else {
                LIns* ecIns = m_contextP;

                // TODO
                // if with statement are implemented, we should load re-implement loading env loading source
                LIns* envIns = m_out->insLoad(LIR_ldp, ecIns, ExecutionContext::offsetOfEnvironment(), 1, LOAD_NORMAL);

                for (int i = 0; i < irSetVar->upVarIndex(); i ++) {
                    envIns = m_out->insLoad(LIR_ldp, envIns, LexicalEnvironment::offsetofOuterEnvironment(), 1, LOAD_NORMAL);
                }

                LIns* record = m_out->insLoad(LIR_ldp, envIns, LexicalEnvironment::offsetofRecord(), 1, LOAD_NORMAL);
                LIns* vectorDataPointer = m_out->insLoad(LIR_ldp, record, (DeclarativeEnvironmentRecord::offsetofActivationData() + ESIdentifierVector::offsetofData()), 1, LOAD_NORMAL);
                m_out->insStore(LIR_ste, boxedSource, vectorDataPointer, sizeof(ESIdentifierVectorStdItem) * irSetVar->localVarIndex() + sizeof(InternalAtomicString), 1);
            }
            return source;
        }
    case ESIR::Opcode::GetVarGenericWithoutException:
        {
            INIT_ESIR(GetVarGenericWithoutException);
            LIns* bytecode = m_out->insImmP(irGetVarGenericWithoutException->originalGetByIdByteCode());
            LIns* args[] = {bytecode, m_contextP, m_instanceP};
            return m_out->insCall(&getByIdWithoutExceptionOpCallInfo, args);
        }
    case ESIR::Opcode::GetVarGeneric:
        {
            INIT_ESIR(GetVarGeneric);
            LIns* bytecode = m_out->insImmP(irGetVarGeneric->originalGetByIdByteCode());
            LIns* cachedIdentifierCacheInvalidationCheckCount = m_out->insLoad(LIR_ldi, bytecode, offsetof(GetById, m_identifierCacheInvalidationCheckCount), 1, LOAD_NORMAL);
            LIns* instanceIdentifierCacheInvalidationCheckCount = m_out->insLoad(LIR_ldi, m_instanceP, ESVMInstance::offsetOfIdentifierCacheInvalidationCheckCount(), 1, LOAD_NORMAL);
            LIns* cachedSlot = m_out->insLoad(LIR_ldp, bytecode, offsetof(GetById, m_cachedSlot), 1, LOAD_NORMAL);
            LIns* phi = m_out->insAlloc(sizeof(ESValue));
            LIns* checkIfCacheHit = m_out->ins2(LIR_eqi, instanceIdentifierCacheInvalidationCheckCount, cachedIdentifierCacheInvalidationCheckCount);
            LIns* jumpIfCacheHit = m_out->insBranch(LIR_jt, checkIfCacheHit, (LIns*)nullptr);

            LIns* slowPath = m_out->ins0(LIR_label);
#if 1
            LIns* args[] = {bytecode, m_contextP};
            LIns* resolvedSlot = m_out->insCall(&contextResolveBindingCallInfo, args);
            LIns* resolvedResult = m_out->insLoad(LIR_lde, resolvedSlot, 0, 1, LOAD_NORMAL);
            m_out->insStore(LIR_ste, resolvedResult, phi, 0 , 1);

            LIns* cachingLookuped = m_out->insStore(LIR_ste, resolvedResult, cachedSlot, 0, 1);
            LIns* storeCheckCount = m_out->insStore(LIR_sti, instanceIdentifierCacheInvalidationCheckCount, bytecode, offsetof(GetById, m_identifierCacheInvalidationCheckCount), 1);
            LIns* jumpToJoin = m_out->insBranch(LIR_j, nullptr, (LIns*)nullptr);
#else
            JIT_LOG(cachedIdentifierCacheInvalidationCheckCount, "GetVarGeneric Cache Miss");
            generateOSRExit(irGetVarGeneric->targetIndex());
#endif

            LIns* fastPath = m_out->ins0(LIR_label);
            jumpIfCacheHit->setTarget(fastPath);
            LIns* cachedResult = m_out->insLoad(LIR_lde, cachedSlot, 0, 1, LOAD_NORMAL);
            m_out->insStore(LIR_ste, cachedResult, phi, 0 , 1);
#if 1
            LIns* pathJoin = m_out->ins0(LIR_label);
            jumpToJoin->setTarget(pathJoin);
            // LIns* ret = m_out->ins3(LIR_cmovd, checkIfCacheHit, cachedResult, resolvedResult);
            LIns* ret = m_out->insLoad(LIR_lde, phi, 0, 1, LOAD_NORMAL);
            return ret;
#else
            return cachedResult;
#endif
        }
    case ESIR::Opcode::GetGlobalVarGeneric:
        {
            INIT_ESIR(GetGlobalVarGeneric);
            /*
            // code for debug
            LIns* byteCode = m_out->insImmP(irGetGlobalVarGeneric->byteCode());
            LIns* globalObject = m_globalObjectP;
            LIns* args[] = {byteCode, globalObject};
            return m_out->insCall(&getByGlobalIndexOpCallInfo, args);
            */
            size_t gapToHiddenClassData = escargot::ESObject::offsetOfHiddenClassData() + ESValueVector::offsetOfData();
            LIns* hiddenClassData = m_out->insLoad(LIR_ldp, m_globalObjectP, gapToHiddenClassData, 1, LOAD_NORMAL);
            LIns* readResult = m_out->insLoad(LIR_lde, hiddenClassData, irGetGlobalVarGeneric->byteCode()->m_index * sizeof(ESValue), 1, LOAD_NORMAL);
            return readResult;
        }
    case ESIR::Opcode::SetVarGeneric:
        {
            INIT_ESIR(SetVarGeneric);

            LIns* source = getTmpMapping(irSetVarGeneric->sourceIndex());
            LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irSetVarGeneric->sourceIndex()));

            LIns* bytecode = m_out->insImmP(irSetVarGeneric->originalPutByIdByteCode());
            LIns* cachedIdentifierCacheInvalidationCheckCount = m_out->insLoad(LIR_ldi, bytecode, offsetof(SetById, m_identifierCacheInvalidationCheckCount), 1, LOAD_NORMAL);
            LIns* instanceIdentifierCacheInvalidationCheckCount = m_out->insLoad(LIR_ldi, m_instanceP, ESVMInstance::offsetOfIdentifierCacheInvalidationCheckCount(), 1, LOAD_NORMAL);
            LIns* checkIfCacheHit = m_out->ins2(LIR_eqi, instanceIdentifierCacheInvalidationCheckCount, cachedIdentifierCacheInvalidationCheckCount);
            LIns* jumpIfCacheHit = m_out->insBranch(LIR_jt, checkIfCacheHit, (LIns*)nullptr);
#if 1
            // JIT_LOG(bytecode, "SetVarGeneric Cache Miss");
            LIns* args[] = {bytecode, m_contextP};
            LIns* resolvedSlot = m_out->insCall(&setVarContextResolveBindingCallInfo, args);
            LIns* checkIfSlotIsNull = m_out->ins2(LIR_eqp, resolvedSlot, m_zeroP);
            LIns* jumpIfSlotIsNull = m_out->insBranch(LIR_jt, checkIfSlotIsNull, (LIns*)nullptr);

            // JIT_LOG(bytecode, "SetVarGeneric Cache Miss->resolveBinding->validSlot");
            m_out->insStore(LIR_stp, resolvedSlot, bytecode, offsetof(SetById, m_cachedSlot), 1);
            m_out->insStore(LIR_sti, instanceIdentifierCacheInvalidationCheckCount, bytecode, offsetof(SetById, m_identifierCacheInvalidationCheckCount), 1);
            m_out->insStore(LIR_ste, boxedSource, resolvedSlot, 0, 1);
            LIns* jumpToEnd1 = m_out->insBranch(LIR_j, nullptr, (LIns*)nullptr);

            LIns* labelDeclareVariable = m_out->ins0(LIR_label);
            jumpIfSlotIsNull->setTarget(labelDeclareVariable);
            // JIT_LOG(bytecode, "SetVarGeneric Cache Miss->resolveBinding->emptySlot");
            LIns* args2[] = {boxedSource, bytecode, m_globalObjectP, m_contextP};
            m_out->insCall(&setVarDefineDataPropertyCallInfo, args2);
            LIns* jumpToEnd2 = m_out->insBranch(LIR_j, nullptr, (LIns*)nullptr);
#else
            JIT_LOG(bytecode, "SetVarGeneric Cache Miss");
            generateOSRExit(irSetVarGeneric->m_targetIndex);
#endif
            LIns* fastPath = m_out->ins0(LIR_label);
            jumpIfCacheHit->setTarget(fastPath);
            // JIT_LOG(bytecode, "SetVarGeneric Cache Hit");
            LIns* cachedSlot = m_out->insLoad(LIR_ldp, bytecode, offsetof(SetById, m_cachedSlot), 1, LOAD_NORMAL);
            m_out->insStore(LIR_ste, boxedSource, cachedSlot, 0, 1);
            LIns* labelEnd = m_out->ins0(LIR_label);
            jumpToEnd1->setTarget(labelEnd);
            jumpToEnd2->setTarget(labelEnd);

            return source;
        }
    case ESIR::Opcode::SetGlobalVarGeneric:
        {
            INIT_ESIR(SetGlobalVarGeneric);
            // code for debug
            /*
            LIns* source = getTmpMapping(irSetGlobalVarGeneric->sourceIndex());
            LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irSetGlobalVarGeneric->sourceIndex()));
            LIns* byteCode = m_out->insImmP(irSetGlobalVarGeneric->byteCode());
            LIns* args[] = {boxedSource, byteCode, m_globalObjectP};
            m_out->insCall(&setByGlobalIndexOpCallInfo, args);
            */
            LIns* source = getTmpMapping(irSetGlobalVarGeneric->sourceIndex());
            LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irSetGlobalVarGeneric->sourceIndex()));
            size_t gapToHiddenClassData = escargot::ESObject::offsetOfHiddenClassData() + ESValueVector::offsetOfData();
            LIns* hiddenClassData = m_out->insLoad(LIR_ldp, m_globalObjectP, gapToHiddenClassData, 1, LOAD_NORMAL);
            // virtual LIns* insStore(LOpcode op, LIns* value, LIns* base, int32_t d, AccSet accSet) {
            m_out->insStore(LIR_ste, boxedSource, hiddenClassData, irSetGlobalVarGeneric->byteCode()->m_index * sizeof(ESValue), 1);
            return source;
        }
    case ESIR::Opcode::GetObject:
        {
            INIT_ESIR(GetObject);
            LIns* obj = boxESValue(getTmpMapping(irGetObject->objectIndex()), m_graph->getOperandType(irGetObject->objectIndex()));
            LIns* property = boxESValue(getTmpMapping(irGetObject->propertyIndex()), m_graph->getOperandType(irGetObject->propertyIndex()));
            LIns* args[] = {m_globalObjectP, property, obj};
            return m_out->insCall(&getObjectOpCallInfo, args);
        }
    case ESIR::Opcode::GetObjectPreComputed:
        {
            INIT_ESIR(GetObjectPreComputed);

            // for 64-bit
            // TODO add ifdef

            if (m_graph->getOperandType(irGetObjectPreComputed->objectIndex()).isArrayObjectType()) {
                if (*irGetObjectPreComputed->byteCode()->m_propertyValue == *strings->length.string()) {
                    LIns* obj = getTmpMapping(irGetObjectPreComputed->objectIndex());
                    LIns* length = m_out->insLoad(LIR_ldi, obj, ESArrayObject::offsetOfLength(), 1, LOAD_NORMAL);
                    return boxESValue(length, Type(TypeInt32));
                }
            }

            bool isStringCase = false;
            LIns* objFromOutSide = NULL;
            if (m_graph->getOperandType(irGetObjectPreComputed->objectIndex()).isStringType()) {
                if (*irGetObjectPreComputed->byteCode()->m_propertyValue == *strings->length.string()) {
                    LIns* obj = getTmpMapping(irGetObjectPreComputed->objectIndex());
                    // load m_string from ESString*
                    LIns* stringData = m_out->insLoad(LIR_ldp, obj, ESString::offsetOfStringData(), 1, LOAD_NORMAL);
                    // read length
                    LIns* length = m_out->ins1(LIR_q2i, m_out->insLoad(LIR_lde, stringData, ESStringData::offsetOfLength(), 1, LOAD_NORMAL));
                    return boxESValue(length, Type(TypeInt32));
                }

                LIns* obj = getTmpMapping(irGetObjectPreComputed->objectIndex());
                objFromOutSide = m_out->insImmP(ESVMInstance::currentInstance()->globalObject()->stringObjectProxy());
                m_out->insStore(LIR_stp, obj, objFromOutSide, 0, 1);
                isStringCase = true;
            }


            if (m_graph->getOperandType(irGetObjectPreComputed->objectIndex()).isObjectType() || isStringCase) {
                LIns* result = m_out->insAlloc(sizeof(ESValue));

                // check proto chain
                LIns* obj = getTmpMapping(irGetObjectPreComputed->objectIndex());
                LIns* orgObj = getTmpMapping(irGetObjectPreComputed->objectIndex());

                if (objFromOutSide) {
                    orgObj = obj = objFromOutSide;
                }

                std::vector<LIns*> lblsToFallback;
                LIns* proto = obj;
                for (int i = 0; i < irGetObjectPreComputed->byteCode()->m_cachedhiddenClassChain.size() ; i ++) {
                    if (i != 0)
                        obj = proto;
                    LIns* savedHiddenClass = m_out->insImmP(irGetObjectPreComputed->byteCode()->m_cachedhiddenClassChain[i]);
                    LIns* hiddenClass = m_out->insLoad(LIR_ldp, obj, escargot::ESObject::offsetOfHiddenClass(), 1, LOAD_NORMAL);
                    LIns* checkHiddenClass = m_out->ins2(LIR_eqp, hiddenClass, savedHiddenClass);
                    LIns* jumpToFallBackIfHiddenClassIsNotSame = m_out->insBranch(LIR_jf, checkHiddenClass, (LIns*)nullptr);
                    lblsToFallback.push_back(jumpToFallBackIfHiddenClassIsNotSame);

                    proto = m_out->insLoad(LIR_lde, obj, ESObject::offsetOf__proto__(), 1, LOAD_NORMAL);
                    LIns* maskedValue = m_out->ins2(LIR_andq, proto, m_tagMaskQ);
                    LIns* checkTagged = m_out->ins2(LIR_eqq, maskedValue, m_zeroQ);
                    LIns* jumpIfPointer = m_out->insBranch(LIR_jf, checkTagged, (LIns*)nullptr);
                    lblsToFallback.push_back(jumpIfPointer);
                }

                // read
                /*
                // for debug
                LIns* args[] = {m_out->insImmP((void *)irGetObjectPreComputed->byteCode()->m_cachedIndex), orgObj, obj};
                LIns* readResult = m_out->insCall(&getObjectPreComputedCaseLastPartOpCallInfo, args);
                m_out->insStore(LIR_ste, readResult, result, 0, 1);
                */

                if (irGetObjectPreComputed->byteCode()->m_cachedIndex == SIZE_MAX) {
                    m_out->insStore(LIR_ste, m_undefinedQ, result, 0, 1);
                } else {
                    if (irGetObjectPreComputed->byteCode()->m_cachedhiddenClassChain.back()->propertyInfo(irGetObjectPreComputed->byteCode()->m_cachedIndex).m_flags.m_isDataProperty) {
                        size_t gapToHiddenClassData = escargot::ESObject::offsetOfHiddenClassData() + ESValueVector::offsetOfData();
                        LIns* hiddenClassData = m_out->insLoad(LIR_ldp, obj, gapToHiddenClassData, 1, LOAD_NORMAL);
                        LIns* readResult = m_out->insLoad(LIR_lde, hiddenClassData, irGetObjectPreComputed->byteCode()->m_cachedIndex * sizeof(ESValue), 1, LOAD_NORMAL);
                        m_out->insStore(LIR_ste, readResult, result, 0, 1);
                        // JIT_LOG(readResult, "inline cache works");
                    } else {
                        // call callback
                        LIns* args[] = {m_out->insImmP((void *)irGetObjectPreComputed->byteCode()->m_cachedIndex), orgObj, obj};
                        LIns* readResult = m_out->insCall(&getObjectPreComputedCaseLastPartOpCallInfo, args);
                        m_out->insStore(LIR_ste, readResult, result, 0, 1);
                    }

                }

                LIns* gotoEnd = m_out->insBranch(LIR_j, nullptr, (LIns*)nullptr);

                {
                    LIns* fallbackPath = m_out->ins0(LIR_label);

                    for (int i = 0; i < lblsToFallback.size() ; i ++) {
                        lblsToFallback[i]->setTarget(fallbackPath);
                    }

                    LIns* obj = getTmpMapping(irGetObjectPreComputed->objectIndex());
                    LIns* boxedObj = boxESValue(obj, m_graph->getOperandType(irGetObjectPreComputed->objectIndex()));
                    LIns* globalObject = m_globalObjectP;
                    LIns* args[] = {m_out->insImmP(irGetObjectPreComputed->byteCode()), globalObject, boxedObj};
                    LIns* fallbackResult = m_out->insCall(&getObjectPreComputedCaseOpCallInfo, args);
                    m_out->insStore(LIR_ste, fallbackResult, result, 0, 1);
                }

                LIns* end = m_out->ins0(LIR_label);
                gotoEnd->setTarget(end);

                return m_out->insLoad(LIR_lde, result, 0, 1);
            } else {
                LIns* obj = getTmpMapping(irGetObjectPreComputed->objectIndex());
                LIns* boxedObj = boxESValue(obj, m_graph->getOperandType(irGetObjectPreComputed->objectIndex()));
                LIns* globalObject = m_globalObjectP;
                LIns* args[] = {m_out->insImmP(irGetObjectPreComputed->byteCode()), globalObject, boxedObj};
                return m_out->insCall(&getObjectPreComputedCaseOpCallInfo, args);
            }
        }
    case ESIR::Opcode::GetArrayObject:
        {
            INIT_ESIR(GetArrayObject);

            /*
            // runtime call for debug
            LIns* obj = boxESValue(getTmpMapping(irGetArrayObject->objectIndex()), m_graph->getOperandType(irGetArrayObject->objectIndex()));
            LIns* property = boxESValue(getTmpMapping(irGetArrayObject->propertyIndex()), m_graph->getOperandType(irGetArrayObject->propertyIndex()));
            LIns* args[] = {m_globalObjectP, property, obj};
            return m_out->insCall(&getObjectOpCallInfo, args);
            */

#ifdef ESCARGOT_64
            LIns* obj = getTmpMapping(irGetArrayObject->objectIndex());
            LIns* key = getTmpMapping(irGetArrayObject->propertyIndex());
            ASSERT(m_graph->getOperandType(irGetArrayObject->objectIndex()).isArrayObjectType());
            Type keyType = m_graph->getOperandType(irGetArrayObject->propertyIndex());
            ASSERT(keyType.isDoubleType() || keyType.isInt32Type());
            LIns* gotoEndInDoublePath = nullptr;

            LIns* phi = m_out->insAlloc(sizeof(ESValue));
            if (keyType.isDoubleType()) {
                // if key is int
                LIns* toInt = m_out->ins1(LIR_d2i, key);
                LIns* toDouble = m_out->ins1(LIR_i2d, toInt);

                LIns* checkInt = m_out->ins2(LIR_eqd, key, toDouble);
                LIns* jumpIfInt = m_out->insBranch(LIR_jt, checkInt, (LIns *)nullptr);

                // if key is not int call fallback
                LIns* fallback = m_out->ins0(LIR_label);
                LIns* obj = boxESValue(getTmpMapping(irGetArrayObject->objectIndex()), m_graph->getOperandType(irGetArrayObject->objectIndex()));
                LIns* property = boxESValue(getTmpMapping(irGetArrayObject->propertyIndex()), m_graph->getOperandType(irGetArrayObject->propertyIndex()));
                LIns* args[] = {m_globalObjectP, property, obj};
                LIns* ret = m_out->insCall(&getObjectOpCallInfo, args);
                m_out->insStore(LIR_ste, ret, phi, 0 , 1);
                gotoEndInDoublePath = m_out->insBranch(LIR_j, nullptr, (LIns *)nullptr);

                LIns* fastPath = m_out->ins0(LIR_label);
                jumpIfInt->setTarget(fastPath);
                key = toInt;
            }

            ASSERT(key->isI());
            LIns* length = m_out->insLoad(LIR_ldi, obj, ESArrayObject::offsetOfLength(), 1, LOAD_NORMAL);

            // FIXME in ecmascript, range of index is 0~2^32-1
            // use uint32_t for indexing

            // length-check
            // idx >= 0
            LIns* checkLessThanZero = m_out->ins2(LIR_lti, key, m_out->insImmI(0));
            LIns* jumpLessThanZero = m_out->insBranch(LIR_jt, checkLessThanZero, (LIns*)nullptr);

            // idx < length
            LIns* checkGretherOrEqualThanLength = m_out->ins2(LIR_gei, key, length);
            LIns* jumpGretherOrEqualThanLength = m_out->insBranch(LIR_jt, checkGretherOrEqualThanLength, (LIns*)nullptr);

            // TODO check fastMode
            // LIns* fastMode = m_out->insLoad(LIR_ldc2i, obj, ESArrayObject::offsetOfIsFastMode(), 1, LOAD_NORMAL);

            // read vector
            size_t gapToVector = escargot::ESArrayObject::offsetOfVectorData();
            size_t gapToVectorData = ESValueVector::offsetOfData() + gapToVector;
            LIns* vectorData = m_out->insLoad(LIR_ldp, obj, gapToVectorData, 1, LOAD_NORMAL);
            LIns* ESValueSize = m_out->insImmI(sizeof(ESValue));
            LIns* offset = m_out->ins2(LIR_muli, key, ESValueSize);
            LIns* offsetAsPointer = m_out->ins1(LIR_i2q, offset);
            LIns* newBase = m_out->ins2(LIR_addp, vectorData, offsetAsPointer);
            LIns* loadedValue = m_out->insLoad(LIR_lde, newBase, 0, 1, LOAD_NORMAL);

            // not empty
            LIns* checkEmpty = m_out->ins2(LIR_eqq, loadedValue, m_emptyQ);
            LIns* jumpIfEmpty = m_out->insBranch(LIR_jt, checkEmpty, (LIns*)nullptr);
            m_out->insStore(LIR_ste, loadedValue, phi, 0 , 1);
            LIns* gotoEnd = m_out->insBranch(LIR_j, nullptr, (LIns*)nullptr);


            // error path
            LIns* errorEnd = m_out->ins0(LIR_label);
            jumpIfEmpty->setTarget(errorEnd);
            jumpLessThanZero->setTarget(errorEnd);
            jumpGretherOrEqualThanLength->setTarget(errorEnd);
            LIns* undefinedAsDouble  = m_undefinedQ;
            m_out->insStore(LIR_ste, undefinedAsDouble, phi, 0 , 1);

            // end
            LIns* end = m_out->ins0(LIR_label);
            if (gotoEndInDoublePath)
                gotoEndInDoublePath->setTarget(end);
            gotoEnd->setTarget(end);

            return m_out->insLoad(LIR_lde, phi, 0, 0);
            // TODO-JY : Beloq code cause access-nsieve bad result
            // int32_t gapToVector = (int32_t) escargot::ESArrayObject::offsetOfVectorData();
            // LIns* vectorData = m_out->insLoad(LIR_ldp, obj, gapToVector, 1, LOAD_NORMAL);
            // return m_out->insLoad(LIR_lde, vectorData, irGetArrayObject->propertyIndex() * sizeof(ESValue), 1, LOAD_NORMAL);
#else
            RELEASE_ASSERT_NOT_REACHED();
#endif
        }
    case ESIR::Opcode::SetArrayObject:
        {
            INIT_ESIR(SetArrayObject);
            /*
            // runtime call for debug
            LIns* obj = boxESValue(getTmpMapping(irSetArrayObject->objectIndex()), m_graph->getOperandType(irSetArrayObject->objectIndex()));
            LIns* property = boxESValue(getTmpMapping(irSetArrayObject->propertyIndex()), m_graph->getOperandType(irSetArrayObject->propertyIndex()));
            LIns* source = boxESValue(getTmpMapping(irSetArrayObject->sourceIndex()), m_graph->getOperandType(irSetArrayObject->sourceIndex()));
            LIns* args[] = {source, property, obj};
            m_out->insCall(&setObjectOpCallInfo, args);
            */
#ifdef ESCARGOT_64
            Type propType = m_graph->getOperandType(irSetArrayObject->propertyIndex());

            LIns* obj = getTmpMapping(irSetArrayObject->objectIndex());
            LIns* key = getTmpMapping(irSetArrayObject->propertyIndex());
            LIns* boxedSource = boxESValue(getTmpMapping(irSetArrayObject->sourceIndex()), m_graph->getOperandType(irSetArrayObject->sourceIndex()));
            LIns* gotoEndInDoublePath = nullptr;

            Type keyType = m_graph->getOperandType(irSetArrayObject->propertyIndex());
            ASSERT(keyType.isDoubleType() || keyType.isInt32Type());
            if (keyType.isDoubleType()) {
                // if key is int
                LIns* toInt = m_out->ins1(LIR_d2i, key);
                LIns* toDouble = m_out->ins1(LIR_i2d, toInt);

                LIns* checkInt = m_out->ins2(LIR_eqd, key, toDouble);
                LIns* jumpIfInt = m_out->insBranch(LIR_jt, checkInt, (LIns *)nullptr);

                // if key is not int call fallback
                gotoEndInDoublePath = m_out->insBranch(LIR_j, nullptr, (LIns *)nullptr);

                LIns* fastPath = m_out->ins0(LIR_label);
                jumpIfInt->setTarget(fastPath);
                key = toInt;
            }

            ASSERT(key->isI());
            LIns* length = m_out->insLoad(LIR_ldi, obj, ESArrayObject::offsetOfLength(), 1, LOAD_NORMAL);

            // FIXME in ecmascript, range of index is 0~2^32-1
            // use uint32_t for indexing

            // length-check
            // idx >= 0
            LIns* checkLessThanZero = m_out->ins2(LIR_lti, key, m_out->insImmI(0));
            LIns* jumpLessThanZero = m_out->insBranch(LIR_jt, checkLessThanZero, (LIns*)nullptr);

            // idx < length
            LIns* checkGretherOrEqualThanLength = m_out->ins2(LIR_gei, key, length);
            LIns* jumpGretherOrEqualThanLength = m_out->insBranch(LIR_jt, checkGretherOrEqualThanLength, (LIns*)nullptr);

            // TODO check fastMode
            // LIns* fastMode = m_out->insLoad(LIR_ldc2i, obj, ESArrayObject::offsetOfIsFastMode(), 1, LOAD_NORMAL);

            // write vector
            size_t gapToVector = escargot::ESArrayObject::offsetOfVectorData();
            size_t gatToVectorData = ESValueVector::offsetOfData();
            LIns* vectorData = m_out->insLoad(LIR_ldp, obj, gapToVector + gatToVectorData, 1, LOAD_NORMAL);
            LIns* ESValueSize = m_out->insImmI(sizeof(ESValue));
            LIns* offset = m_out->ins2(LIR_muli, key, ESValueSize);
            LIns* offsetAsPointer = m_out->ins1(LIR_i2q, offset);
            LIns* newBase = m_out->ins2(LIR_addp, vectorData, offsetAsPointer);
            m_out->insStore(LIR_ste, boxedSource , newBase, 0, 1);
            LIns* gotoEnd = m_out->insBranch(LIR_j, nullptr, (LIns*)nullptr);

            // error path
            LIns* errorEnd = m_out->ins0(LIR_label);
            jumpLessThanZero->setTarget(errorEnd);
            jumpGretherOrEqualThanLength->setTarget(errorEnd);
            if (gotoEndInDoublePath)
                gotoEndInDoublePath->setTarget(errorEnd);
            {
                LIns* obj = boxESValue(getTmpMapping(irSetArrayObject->objectIndex()), m_graph->getOperandType(irSetArrayObject->objectIndex()));
                LIns* property = boxESValue(getTmpMapping(irSetArrayObject->propertyIndex()), m_graph->getOperandType(irSetArrayObject->propertyIndex()));
                LIns* source = boxESValue(getTmpMapping(irSetArrayObject->sourceIndex()), m_graph->getOperandType(irSetArrayObject->sourceIndex()));
                LIns* args[] = {source, property, obj};
                m_out->insCall(&setObjectOpCallInfo, args);
            }
            // end
            LIns* end = m_out->ins0(LIR_label);
            gotoEnd->setTarget(end);

            return getTmpMapping(irSetArrayObject->sourceIndex());
#else
            RELEASE_ASSERT_NOT_REACHED();
#endif

        }
    case ESIR::Opcode::GetStringByIndex:
        {
            INIT_ESIR(GetStringByIndex);

#ifdef ESCARGOT_64
            Type keyType = m_graph->getOperandType(irGetStringByIndex->propertyIndex());
            LIns* obj = getTmpMapping(irGetStringByIndex->objectIndex());
            LIns* key = getTmpMapping(irGetStringByIndex->propertyIndex());
            if (keyType.isInt32Type()) {

                /*
                code for debug
                {
                    LIns* obj = boxESValue(getTmpMapping(irGetStringByIndex->objectIndex()), m_graph->getOperandType(irGetStringByIndex->objectIndex()));
                    LIns* property = boxESValue(getTmpMapping(irGetStringByIndex->propertyIndex()), m_graph->getOperandType(irGetStringByIndex->propertyIndex()));
                    LIns* args[] = {m_globalObjectP, property, obj};
                    m_out->insCall(&getObjectOpCallInfo, args);
                }
                */

                LIns* result = m_out->insAlloc(sizeof (ESValue));

                // FIXME in ecmascript, range of index is 0~2^32-1
                // use uint32_t for indexing

                // load m_string from ESString*
                LIns* stringData = m_out->insLoad(LIR_ldp, obj, ESString::offsetOfStringData(), 1, LOAD_NORMAL);

                // check m_string == NULL
                // LIns* checkNULL = m_out->ins2(LIR_eqp, stringData, m_out->insImmP(0));

                // jump to fallback path if m_string is NULL
                // LIns* jumpToFallbackIfNull = m_out->insBranch(LIR_jt, checkNULL, (LIns*)nullptr);

                // check key >= 0
                LIns* checkLessThanZero = m_out->ins2(LIR_lti, key, m_out->insImmI(0));
                LIns* jumpLessThanZero = m_out->insBranch(LIR_jt, checkLessThanZero, (LIns*)nullptr);

                // check key < length
                LIns* length = m_out->insLoad(LIR_lde, stringData, ESStringData::offsetOfLength(), 1, LOAD_NORMAL);
                LIns* checkGretherOrEqualThanLength = m_out->ins2(LIR_gei, key, m_out->ins1(LIR_q2i, length));
                LIns* jumpGretherOrEqualThanLength = m_out->insBranch(LIR_jt, checkGretherOrEqualThanLength, (LIns*)nullptr);

                // read data of m_string
                LIns* stringDataPointer = m_out->insLoad(LIR_ldp, stringData, ESStringData::offsetOfData(), 1, LOAD_NORMAL);

                // move string pointer
                stringDataPointer = m_out->ins2(LIR_addp, stringDataPointer, m_out->ins1(LIR_i2q, key));

                // read char in m_string
                // FIXME read as unsigned short!
                LIns* stringPiece = m_out->insLoad(LIR_lds2i, stringDataPointer, 0, LOAD_NORMAL);

                // test
                // if (LIKELY(char < ESCARGOT_ASCII_TABLE_MAX)) {
                LIns* checkASCII = m_out->ins2(LIR_gei, stringPiece, m_out->insImmI(ESCARGOT_ASCII_TABLE_MAX));
                LIns* jumpIfNotASCII = m_out->insBranch(LIR_jt, checkASCII, (LIns*)nullptr);

                // return strings->asciiTable[c].string();
                LIns* pointerOfAsciiTable = m_out->insImmP(strings->asciiTable);
                LIns* offset = m_out->ins2(LIR_muli, stringPiece, m_out->insImmI(sizeof(InternalAtomicString)));
                pointerOfAsciiTable = m_out->ins2(LIR_addp, pointerOfAsciiTable, m_out->ins1(LIR_i2q, offset));
                LIns* stringResult = m_out->insLoad(LIR_ldp, pointerOfAsciiTable, InternalAtomicString::offsetOfString(), 1, LOAD_NORMAL);
                LIns* stringResultAsQuad = boxESValue(stringResult, Type(TypeString));
                m_out->insStore(LIR_ste, stringResultAsQuad, result, 0, 1);

                // operation end. go to end path
                LIns* gotoOperationEnd = m_out->insBranch(LIR_j, nullptr, (LIns*)nullptr);

                {
                    LIns* fallbackPath = m_out->ins0(LIR_label);
                    // jumpToFallbackIfNull->setTarget(fallbackPath);
                    jumpLessThanZero->setTarget(fallbackPath);
                    jumpGretherOrEqualThanLength->setTarget(fallbackPath);
                    jumpIfNotASCII->setTarget(fallbackPath);

                    // fallback path
                    LIns* obj = boxESValue(getTmpMapping(irGetStringByIndex->objectIndex()), m_graph->getOperandType(irGetStringByIndex->objectIndex()));
                    LIns* property = boxESValue(getTmpMapping(irGetStringByIndex->propertyIndex()), m_graph->getOperandType(irGetStringByIndex->propertyIndex()));
                    LIns* args[] = {m_globalObjectP, property, obj};
                    LIns* fallbackRet = m_out->insCall(&getObjectOpCallInfo, args);
                    m_out->insStore(LIR_ste, fallbackRet, result, 0, 1);
                }

                LIns* end = m_out->ins0(LIR_label);
                gotoOperationEnd->setTarget(end);
                return m_out->insLoad(LIR_lde, result, 0, 1, LOAD_NORMAL);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
#else
            RELEASE_ASSERT_NOT_REACHED();
#endif
        }
    case ESIR::Opcode::SetObject:
        {
            INIT_ESIR(SetObject);
            LIns* obj = boxESValue(getTmpMapping(irSetObject->objectIndex()), m_graph->getOperandType(irSetObject->objectIndex()));
            LIns* prop = boxESValue(getTmpMapping(irSetObject->propertyIndex()), m_graph->getOperandType(irSetObject->propertyIndex()));
            LIns* source = getTmpMapping(irSetObject->sourceIndex());
            LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irSetObject->targetIndex()));

            LIns* args[] = {boxedSource, prop, obj};
            m_out->insCall(&setObjectOpCallInfo, args);
            return source;
        }
    case ESIR::Opcode::SetObjectPreComputed:
        {
            INIT_ESIR(SetObjectPreComputed);
            LIns* obj = boxESValue(getTmpMapping(irSetObjectPreComputed->objectIndex()), m_graph->getOperandType(irSetObjectPreComputed->objectIndex()));
            LIns* source = getTmpMapping(irSetObjectPreComputed->sourceIndex());
            LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irSetObjectPreComputed->targetIndex()));
            LIns* args[] = {boxedSource, m_out->insImmP(irSetObjectPreComputed->byteCode()), m_globalObjectP, obj};
            m_out->insCall(&setObjectPreComputedCaseOpCallInfo, args);
            return source;
        }
    case ESIR::Opcode::ToNumber:
        {
            INIT_ESIR(ToNumber);
            LIns* source = getTmpMapping(irToNumber->sourceIndex());
            Type srcType = m_graph->getOperandType(irToNumber->sourceIndex());
            if (srcType.isInt32Type() || srcType.isDoubleType())
                return source;
            else
                return nullptr;
        }
    case ESIR::Opcode::Increment:
        {
            INIT_ESIR(Increment);
            INIT_UNARY_ESIR(Increment);
            if (valueType.isInt32Type()) {
                LIns* one = m_out->insImmI(1);
                LIns* ret = m_out->ins2(LIR_addi, value, one);
                return ret;
            } else if (valueType.isDoubleType()) {
                LIns* one = m_out->insImmD(1);
                return m_out->ins2(LIR_addd, value, one);
            } else
                return nullptr;
        }
    case ESIR::Opcode::Decrement:
        {
            INIT_ESIR(Decrement);
            INIT_UNARY_ESIR(Decrement);
            if (valueType.isInt32Type()) {
                LIns* one = m_out->insImmI(1);
                LIns* ret = m_out->ins2(LIR_subi, value, one);
                return ret;
            } else if (valueType.isDoubleType()) {
                LIns* one = m_out->insImmD(1);
                return m_out->ins2(LIR_subd, value, one);
            } else
                return nullptr;
        }
    case ESIR::Opcode::BitwiseNot:
        {
            INIT_ESIR(BitwiseNot);
            INIT_UNARY_ESIR(BitwiseNot);
            if (valueType.isInt32Type())
                return m_out->ins1(LIR_noti, value);
            else if (valueType.isDoubleType())
                return m_out->ins1(LIR_noti, m_out->ins1(LIR_d2i, value));
            else {
                CALL_UNARY_ESIR(BitwiseNot, bitwiseNotOpCallInfo);
                return unboxedResult;
            }
        }
    case ESIR::Opcode::LogicalNot:
        {
            INIT_ESIR(LogicalNot);
            INIT_UNARY_ESIR(LogicalNot);
            if (valueType.isNullType() || valueType.isUndefinedType())
                return m_true;
            else if (valueType.isInt32Type()) {
                return m_out->ins2(LIR_eqi, m_zeroI, value);
            }
            else if (valueType.isBooleanType())
                return m_out->ins2(LIR_xori, value, m_oneI);
            else {
                CALL_UNARY_ESIR(LogicalNot, logicalNotOpCallInfo);
                return unboxedResult;
            }
        }
    case ESIR::Opcode::UnaryMinus:
        {
            INIT_ESIR(UnaryMinus);
            INIT_UNARY_ESIR(UnaryMinus);
            if (valueType.isInt32Type())
                return m_out->ins1(LIR_negi, value);
            else if (valueType.isDoubleType())
                return m_out->ins1(LIR_negd, value);
            else
                return nullptr;
        }
    case ESIR::Opcode::TypeOf:
        {
            INIT_ESIR(TypeOf);
            INIT_UNARY_ESIR(TypeOf);
            CALL_UNARY_ESIR(TypeOf, typeOfOpCallInfo);
            return unboxedResult;
        }
    case ESIR::Opcode::AllocPhi:
        {
            INIT_ESIR(AllocPhi);
            LIns* phi = m_out->insAlloc(sizeof(ESValue));
            return phi;
        }
    case ESIR::Opcode::StorePhi:
        {
            INIT_ESIR(StorePhi);
            LIns* source = getTmpMapping(irStorePhi->sourceIndex());
            LIns* phi = getTmpMapping(irStorePhi->allocPhiIndex());

            Type srcType = m_graph->getOperandType(irStorePhi->sourceIndex());
            if (srcType.isInt32Type() || srcType.isBooleanType())
                return m_out->insStore(LIR_sti, source, phi, 0 , 1);
            else if (srcType.isObjectType() || srcType.isArrayObjectType() || srcType.isStringType() || srcType.isFunctionObjectType())
                return m_out->insStore(LIR_stp, source, phi, 0 , 1);
            else
                return nullptr;
        }
    case ESIR::Opcode::LoadPhi:
        {
            INIT_ESIR(LoadPhi);
            LIns* phi = getTmpMapping(irLoadPhi->allocPhiIndex());

            // TODO use type of srcIndex1
            // currently, if typeof srcIndex0 and srcIndex1 are diffrent,
            // complie will failed in ESGraphTypeInference.cpp
            Type consequentType = m_graph->getOperandType(irLoadPhi->srcIndex0());

            // TODO implement another types
            if (consequentType.isInt32Type() || consequentType.isBooleanType())
                return m_out->insLoad(LIR_ldi, phi, 0, 1, LOAD_NORMAL);
            else if (consequentType.isObjectType() || consequentType.isArrayObjectType() || consequentType.isStringType() || consequentType.isFunctionObjectType())
                return m_out->insLoad(LIR_lde, phi, 0, 1, LOAD_NORMAL);
            else
                return nullptr;
        }
    case ESIR::Opcode::CreateObject:
        {
            INIT_ESIR(CreateObject);
            LIns* keyCount = m_out->insImmI(irCreateObject->keyCount());
            LIns* args[] = {keyCount};
            return m_out->insCall(&createObjectCallInfo, args);
        }
    case ESIR::Opcode::CreateArray:
        {
            INIT_ESIR(CreateArray);
            LIns* keyCount = m_out->insImmI(irCreateArray->keyCount());
            LIns* args[] = {keyCount};
            return m_out->insCall(&createArrayCallInfo, args);
        }
    case ESIR::Opcode::InitObject:
        {
            INIT_ESIR(InitObject);
            LIns* boxedObj = boxESValue(getTmpMapping(irInitObject->objectIndex()), m_graph->getOperandType(irInitObject->objectIndex()));
            LIns* boxedKey = boxESValue(getTmpMapping(irInitObject->keyIndex()), m_graph->getOperandType(irInitObject->keyIndex()));
            LIns* boxedVal = boxESValue(getTmpMapping(irInitObject->sourceIndex()), m_graph->getOperandType(irInitObject->sourceIndex()));
            LIns* args[] = {boxedVal, boxedKey, boxedObj};
            return m_out->insCall(&initObjectCallInfo, args);
        }
    case ESIR::Opcode::InitArrayObject:
        {
            INIT_ESIR(InitArrayObject);
            InitArrayObjectIR* irInitObject = irInitArrayObject;
            // TODO : Check fast mode. If not, call runtime function
            LIns* obj = getTmpMapping(irInitObject->objectIndex());
            if (obj->isI())
                obj = m_out->ins1(LIR_i2q, obj);
            LIns* key = getTmpMapping(irInitObject->keyIndex());
            LIns* source = getTmpMapping(irInitObject->sourceIndex());
            Type srcType = m_graph->getOperandType(irInitObject->sourceIndex());

            LIns* invalidIndexValue = m_out->insImmI(ESValue::ESInvalidIndexValue);
            // FIXME
            if (key->isD())
                key = m_out->ins1(LIR_d2i, key);
            LIns* checkInvalidIndex = m_out->ins2(LIR_eqi, key, invalidIndexValue);
            LIns* jf1 = m_out->insBranch(LIR_jf, checkInvalidIndex, (LIns*)nullptr);
            JIT_LOG(key, "InitArrayObject: key is invalid(Too Big)");
            // TODO : If key is invalid, call runtime function

            LIns* label1 = m_out->ins0(LIR_label);
            jf1->setTarget(label1);

            LIns* length  = m_out->insLoad(LIR_ldi, obj, ESArrayObject::offsetOfLength(), 1, LOAD_NORMAL);
            LIns* checkBound = m_out->ins2(LIR_gti, key, length);
            LIns* jf2 = m_out->insBranch(LIR_jf, checkBound, (LIns*)nullptr);
            JIT_LOG(key, "InitArrayObject: bound error ");

            LIns* label2 = m_out->ins0(LIR_label);
            jf2->setTarget(label2);
            LIns* ESValueSize = m_out->insImmI(sizeof(ESValue));
            LIns* offset = m_out->ins2(LIR_muli, key, ESValueSize);
            LIns* offsetP = m_out->ins1(LIR_i2q, offset);

            size_t gapToVector = escargot::ESArrayObject::offsetOfVectorData();
            size_t gatToVectorData = ESValueVector::offsetOfData();
            LIns* vector = m_out->insLoad(LIR_ldp, obj, gapToVector, 1, LOAD_NORMAL);
            LIns* vectorData = m_out->ins2(LIR_addp, vector, m_out->insImmP((void *)gatToVectorData));

            // LIns* vectorData = m_out->insLoad(LIR_ldp, obj, ESArrayObject::offsetOfVectorData(), 1, LOAD_NORMAL);
            LIns* valuePtr = m_out->ins2(LIR_addp, vectorData, offsetP);
            LIns* asInt32 = m_out->insLoad(LIR_lde, valuePtr, ESValue::offsetOfAsInt64(), 1, LOAD_NORMAL);
            LIns* checkEmptyValue = m_out->ins2(LIR_eqq, asInt32, m_zeroQ);
            LIns* jf3 = m_out->insBranch(LIR_jt, checkEmptyValue, (LIns*)nullptr);
            JIT_LOG(key, "InitArrayObject: NonEmptyValue ");

            LIns* label3 = m_out->ins0(LIR_label);
            jf3->setTarget(label3);

            // LIns* proto = m_out->insLoad(LIR_lde, obj, ESObject::offsetOf__proto__(), 1, LOAD_NORMAL);
            // ToDo(JMP) : ReadOnly Check

            LIns* boxedSrc = boxESValue(source, srcType);
            LIns* offset1 = m_out->ins2(LIR_muli, key, ESValueSize);
            LIns* offset1P = m_out->ins1(LIR_i2q, offset1);
            LIns* valuePtr1 = m_out->ins2(LIR_addq, vectorData, offset1P);
            LIns* ret =  m_out->insStore(LIR_ste, boxedSrc, valuePtr1, 0, 1);
            return source;
        }
    default:
        {
            return nullptr;
        }
#undef INIT_ESIR
    }
}

bool NativeGenerator::nanojitCodegen(ESVMInstance* instance)
{
#ifndef NDEBUG
    if (ESVMInstance::currentInstance()->m_useVerboseWriter) {
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("LIR Generation Started\n");
    }
#endif
    m_out->ins0(LIR_start);
    for (int i = 0; i < nanojit::NumSavedRegs; ++i)
        m_out->insParam(i, 1);

#ifdef ESCARGOT_64
    m_tagMaskQ = m_out->insImmQ(TagMask);
    m_booleanTagQ = m_out->insImmQ(TagBitTypeOther | TagBitBool);
    m_booleanTagComplementQ = m_out->insImmQ(~(TagBitTypeOther | TagBitBool));
    m_intTagQ = m_out->insImmQ(TagTypeNumber);
    m_intTagComplementQ = m_out->insImmQ(~TagTypeNumber);
    m_doubleEncodeOffsetQ = m_out->insImmQ(DoubleEncodeOffset);
    m_undefinedQ = m_out->insImmQ(ValueUndefined);
    m_nullQ = m_out->insImmQ(ValueNull);
    m_emptyQ = m_out->insImmQ(ValueEmpty);
    m_emptyD = m_out->ins1(LIR_qasd, m_emptyQ);
    m_zeroQ = m_out->insImmQ(0);
#endif
    m_zeroD = m_out->insImmD(0);
    m_zeroP = m_out->insImmP(0);
    m_oneI = m_out->insImmI(1);
    m_zeroI = m_out->insImmI(0);
    m_true = m_oneI;
    m_false = m_zeroI;
    m_thisValueP = m_out->insAlloc(sizeof(ESValue));
    m_instanceP = m_out->insImmP(ESVMInstance::currentInstance());
    m_contextP = m_out->insLoad(LIR_ldp, m_instanceP, ESVMInstance::offsetOfCurrentExecutionContext(), 1, LOAD_NORMAL);
    m_cachedDeclarativeEnvironmentRecordESValueP = m_out->insLoad(LIR_ldp, m_contextP, ExecutionContext::offsetofcachedDeclarativeEnvironmentRecordESValue(), 1, LOAD_NORMAL);
    m_globalObjectP = m_out->insImmP(ESVMInstance::currentInstance()->globalObject());
    m_out->defaultInsToExtendLife.push_back(m_thisValueP);
    m_out->defaultInsToExtendLife.push_back(m_contextP);
    m_out->defaultInsToExtendLife.push_back(m_cachedDeclarativeEnvironmentRecordESValueP);

#ifdef ESCARGOT_64
    m_out->insStore(LIR_ste, m_emptyQ, m_thisValueP, 0, 1);
#endif

    // JIT_LOG(m_true, "Start executing JIT function");

    // Generate code for each IRs
    for (size_t i = 0; i < m_graph->basicBlockSize(); i++) {
        ESBasicBlock* block = m_graph->basicBlock(i);
        block->setLabel(m_out->ins0(LIR_label));
        for (size_t j = 0; j < block->instructionSize(); j++) {
            ESIR* ir = block->instruction(j);
            LIns* generatedLIns = nanojitCodegen(ir);
            if (!generatedLIns) {
                LOG_VJ("Cannot generate code for JIT IR `%s` in ESJITBackEnd\n", ir->getOpcodeName());
#ifndef NDEBUG
                if (ESVMInstance::currentInstance()->m_useVerboseWriter) {
                    printf("LIR Generation Aborted\n");
                    printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                }
#endif
                return false;
            }
            if (ir->returnsESValue() && ir->m_targetIndex >= 0) {
                // Even if IR has "returnsESValue" tag, there is a possibility that it has invalid targetIndex (e.g. CreateFunction)
                Type type = m_graph->getOperandType(ir->m_targetIndex);
                generatedLIns = generateTypeCheck(generatedLIns, type, ir->m_targetIndex);
                if (!generatedLIns) {
                    LOG_VJ("Cannot generate type check code for type %s(0x%x) in ESJIT Backend\n", type.getESIRTypeName(), type.type());
                    return false;
                }
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

    m_f->lastIns = m_out->insGuard(LIR_x, nullptr, rec);

#ifndef NDEBUG
    if (ESVMInstance::currentInstance()->m_useVerboseWriter) {
        printf("LIR Generation Ended\n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    }
#endif


    return true;
}

JITFunction NativeGenerator::nativeCodegen() {

    m_assm->compile(m_f, *m_alloc, false verbose_only(, m_f->lirbuf->printer));
    if (m_assm->error() != None) {
        LOG_VJ("nanojit failed to generate native code (Error : ", m_assm->error());
        switch (m_assm->error()) {
        case StackFull:     LOG_VJ("StackFull)\n");     break;
        case UnknownBranch: LOG_VJ("UnknownBranch)\n"); break;
        case BranchTooFar : LOG_VJ("BranchTooFar)\n");  break;
        default: RELEASE_ASSERT_NOT_REACHED();
        }
        return nullptr;
    }

    return reinterpret_cast<JITFunction>(m_f->code());
}

JITFunction generateNativeFromIR(ESGraph* graph, ESVMInstance* instance)
{
    NativeGenerator gen(graph);

    unsigned long time1 = ESVMInstance::currentInstance()->tickCount();
    if (!gen.nanojitCodegen(instance))
        return nullptr;
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
    out.ins1(LIR_rete, result);

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
    // Fragmento *fragmento;
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
