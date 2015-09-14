#ifndef ESJITBackend_h
#define ESJITBackend_h

#include "nanojit.h"

namespace escargot {

class ESVMInstance;

namespace ESJIT {

class ESGraph;
class ESIR;

typedef ESValue (*JITFunction)(ESVMInstance*);

class NativeGenerator {
public:
    NativeGenerator(ESGraph* graph);
    JITFunction codegen();
    void nanojitCodegen(ESIR* ir);

private:
    ESGraph* m_graph;

    nanojit::LogControl m_lc;
    nanojit::Config m_config;
    nanojit::Allocator* m_alloc;
    nanojit::CodeAlloc* m_codeAlloc;
    nanojit::Assembler* m_assm;
    nanojit::LirBuffer* m_buf;
    nanojit::Fragment* m_f;
    nanojit::LirBufWriter m_out;
};

JITFunction generateNativeFromIR(ESGraph* graph);

JITFunction addDouble();
int nanoJITTest();

}}
#endif
