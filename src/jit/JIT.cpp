#include "Escargot.h"
#include "JIT.h"

#include "nanojit.h"

namespace escargot {

static unsigned long getLongTickCount()
{
    struct timespec timespec;
    clock_gettime(CLOCK_MONOTONIC,&timespec);
    return (unsigned long)(timespec.tv_sec * 1000000L + timespec.tv_nsec/1000);
}

int nanoJITTest()
{
    unsigned long start = getLongTickCount();

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
    Assembler *assm = new  Assembler(*codeAlloc, *alloc, *alloc, &lc, config);
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

    unsigned long end = getLongTickCount();
    printf("Took %lf ms\n",(end-start)/1000.0);

    typedef int32_t (*AddTwoFn)(int32_t);
    AddTwoFn fn = reinterpret_cast<AddTwoFn>(f->code());
    printf("2 + 5 = %d\n", fn(5));

    return 0;
}

}
