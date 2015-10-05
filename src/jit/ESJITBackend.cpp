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
CallInfo plusOperationCallInfo = CI(plusOperation, CallInfo::typeSig2(ARGTYPE_D, ARGTYPE_D, ARGTYPE_D));
CallInfo minusOperationCallInfo = CI(minusOperation, CallInfo::typeSig2(ARGTYPE_D, ARGTYPE_D, ARGTYPE_D));
#ifndef NDEBUG
CallInfo logCallInfo = CI(jitLogOperation, CallInfo::typeSig1(ARGTYPE_V, ARGTYPE_D));
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
    m_out.insStore(LIR_sti, oneI, m_context, ExecutionContext::offsetofInOSRExit(), 1);
    LIns* bytecode = m_out.insImmQ(currentByteCodeIndex); // FIXME
    return m_out.ins1(LIR_retq, bytecode);
}

LIns* NativeGenerator::generateTypeCheck(LIns* in, Type type, size_t currentByteCodeIndex)
{
#ifndef NDEBUG
    m_out.insComment(".= typecheck start =.");
#endif
    if (type.isInt32Type()) {
#ifdef ESCARGOT_64
        LIns* maskedValue = m_out.ins2(LIR_andq, in, intTagQ);
        LIns* checkIfInt = m_out.ins2(LIR_eqq, maskedValue, intTagQ);
        LIns* jumpIfInt = m_out.insBranch(LIR_jt, checkIfInt, nullptr);
        generateOSRExit(currentByteCodeIndex);
        LIns* normalPath = m_out.ins0(LIR_label);
        jumpIfInt->setTarget(normalPath);
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
        LIns* boxedValue = m_out.ins2(LIR_orq, wideUnboxedValue, intTagQ);
        LIns* boxedValueInDouble = m_out.ins1(LIR_qasd, boxedValue);
        return boxedValueInDouble;
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
        LIns* unboxedValue = m_out.ins2(LIR_andq, boxedValue, intTagComplementQ);
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
        return m_out.insImmI(irConstantInt->value());
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
        LIns* cond = m_out.insImmI(1);
        LIns* jump = m_out.insBranch(LIR_jt, cond, nullptr);
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
        LIns* trueValue = m_out.insImmI(1);
        LIns* compare = m_out.ins2(LIR_eqi, condition, trueValue);
        LIns* jumpFalse = m_out.insBranch(LIR_jf, compare, nullptr);
        //LIns* jumpFalse = m_out.insBranch(LIR_jf, compare, nullptr);
        if (LIns* label = irBranch->falseBlock()->getLabel()) {
            jumpFalse->setTarget(label);
        } else {
            irBranch->falseBlock()->addJumpOrBranchSource(jumpFalse);
        }
        return jumpFalse;
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
        LIns* args[] = {argument};
        m_out.insCall(&logCallInfo, args);
#endif
        return argument;
    }
    case ESIR::Opcode::GetVar:
    {
        INIT_ESIR(GetVar);
#if 0
        return getVarMapping(irGetVar->varIndex());
#else
        return m_out.insLoad(LIR_ldd, m_stackPtr, irGetVar->varIndex() * sizeof(ESValue), 1, LOAD_NORMAL);
#endif
    }
    case ESIR::Opcode::SetVar:
    {
        INIT_ESIR(SetVar);
#if 0
        LIns* source = getTmpMapping(irSetVar->sourceIndex());
        setVarMapping(irSetVar->localVarIndex(), source);
        return source;
#else
        LIns* source = getTmpMapping(irSetVar->sourceIndex());
        LIns* boxedSource = boxESValue(source, m_graph->getOperandType(irSetVar->m_targetIndex));
        return m_out.insStore(LIR_std, boxedSource, m_stackPtr, irSetVar->localVarIndex() * sizeof(ESValue), 1);
#endif
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

    intTagQ = m_out.insImmQ(TagTypeNumber);
    intTagComplementQ = m_out.insImmQ(~TagTypeNumber);
    zeroQ = m_out.insImmQ(0);
    oneI = m_out.insImmI(1);

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
