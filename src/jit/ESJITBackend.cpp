#include "Escargot.h"
#include "ESJITBackend.h"

#include "ESJIT.h"
#include "ESIR.h"
#include "ESGraph.h"

#include "nanojit.h"

namespace escargot {
namespace ESJIT {

using namespace nanojit;

NativeGenerator::NativeGenerator(ESGraph* graph)
    : m_graph(graph),
    m_IRToLInsMapping(graph->tempRegisterSize()),
    m_alloc(new Allocator()),
    m_codeAlloc(new CodeAlloc(&m_config)),
    m_assm(new Assembler(*m_codeAlloc, *m_alloc, *m_alloc, &m_lc, m_config)),
    m_buf(new LirBuffer(*m_alloc)),
    m_f(new Fragment(NULL verbose_only(, 0))),
    m_out(m_buf, m_config)
{
#ifdef DEBUG
    m_lc.lcbits = LC_ReadLIR | LC_Native;
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

LIns* NativeGenerator::nanojitCodegen(ESIR* ir)
{
    switch(ir->opcode()) {
    #define INIT_ESIR(opcode) opcode##IR* ir##opcode = static_cast<opcode##IR*>(ir);
    case ESIR::Opcode::ConstantInt:
    {
        INIT_ESIR(ConstantInt);
        return m_out.insImmI(irConstantInt->value());
    }
    case ESIR::Opcode::LessThan:
    {
        INIT_ESIR(LessThan);
        LIns* left = getMapping(irLessThan->leftIndex());
        LIns* right = getMapping(irLessThan->rightIndex());
        return m_out.ins2(LIR_lti, left, right);
    }
    case ESIR::Opcode::Jump:
    {
        INIT_ESIR(Jump);
        LIns* jump = m_out.insBranch(LIR_j, nullptr, nullptr);
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
        LIns* condition = getMapping(irBranch->operandIndex());
        LIns* zero = m_out.insImmI(0);
        LIns* compare = m_out.ins2(LIR_eqi, condition, zero);
        LIns* jumpTrue = m_out.insBranch(LIR_jt, compare, nullptr);
        //LIns* jumpFalse = m_out.insBranch(LIR_jf, compare, nullptr);
        if (LIns* label = irBranch->falseBlock()->getLabel()) {
            jumpTrue->setTarget(label);
        } else {
            irBranch->falseBlock()->addJumpOrBranchSource(jumpTrue);
        }
        return jumpTrue;
    }
    case ESIR::Opcode::Return:
    {
        LIns* undefined = m_out.insImmD(0);
        return m_out.ins1(LIR_retd, undefined);
    }
    case ESIR::Opcode::ReturnWithValue:
    {
        INIT_ESIR(ReturnWithValue);
        LIns* returnValue = getMapping(irReturnWithValue->returnIndex());
        LIns* returnESValue = m_out.ins1(LIR_i2q, returnValue);
        return m_out.ins1(LIR_retq, returnESValue);
    }
    case ESIR::Opcode::GetArgument:
    {
        INIT_ESIR(GetArgument);
        LIns* quadArgument = m_out.insParam(irGetArgument->argumentIndex(), 0);
        LIns* argument = m_out.ins1(LIR_q2i, quadArgument);
        return argument;
    }
    case ESIR::Opcode::GetVar:
    {
        return nullptr;
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

    // Generate code for each IRs
    for (size_t i = 0; i < m_graph->basicBlockSize(); i++) {
        ESBasicBlock* block = m_graph->basicBlock(i);
        block->setLabel(m_out.ins0(LIR_label));
        for (size_t j = 0; j < block->instructionSize(); j++) {
            ESIR* ir = block->instruction(j);
            LIns* generatedLIns = nanojitCodegen(ir);
            if (!generatedLIns) {
                printf("ERROR: Cannot generate code for JIT IR `%s`\n", ir->getOpcodeName());
            }
            if (ir->m_targetIndex >= 0) {
                // printf("mapping[%d] = %p\n", ir->m_targetIndex, generatedLIns);
                setMapping(ir->m_targetIndex, generatedLIns);
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
        fprintf(stderr, "error compiling fragment\n");
        return nullptr;
    }
    printf("Compilation successful.\n");

    return reinterpret_cast<JITFunction>(m_f->code());
}

JITFunction generateNativeFromIR(ESGraph* graph)
{
    NativeGenerator gen(graph);

    unsigned long time1 = ESVMInstance::tickCount();
    gen.nanojitCodegen();
    unsigned long time2 = ESVMInstance::tickCount();
    JITFunction function = gen.nativeCodegen();
    unsigned long time3 = ESVMInstance::tickCount();
    printf("JIT Compilation Took %lfms, %lfms each for nanojit/native generation\n",
            (time2-time1)/1000.0, (time3-time2)/1000.0);

    return function;
}

JITFunction addDouble()
{
    unsigned long start = ESVMInstance::tickCount();

    using namespace nanojit;
    LogControl lc;
#ifdef DEBUG
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

    unsigned long end = ESVMInstance::tickCount();
    printf("JIT Compilation Took %lf ms\n",(end-start)/1000.0);

    return reinterpret_cast<JITFunction>(f->code());
}

int nanoJITTest()
{
    unsigned long start = ESVMInstance::tickCount();

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

    unsigned long end = ESVMInstance::tickCount();
    printf("Took %lf ms\n",(end-start)/1000.0);

    typedef int32_t (*AddTwoFn)(int32_t);
    AddTwoFn fn = reinterpret_cast<AddTwoFn>(f->code());
    printf("2 + 5 = %d\n", fn(5));

    return 0;
}

}}
