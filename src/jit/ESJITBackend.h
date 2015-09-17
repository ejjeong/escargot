#ifndef ESJITBackend_h
#define ESJITBackend_h

#include "nanojit.h"
#include <vector>

namespace escargot {

class ESVMInstance;

namespace ESJIT {

class ESGraph;
class ESIR;

typedef ESValue (*JITFunction)(ESVMInstance*);

class NativeGenerator {
public:
    NativeGenerator(ESGraph* graph);
    ~NativeGenerator();
    void nanojitCodegen();
    JITFunction nativeCodegen();
    nanojit::LIns* nanojitCodegen(ESIR* ir);


private:
    ESGraph* m_graph;
    void setMapping(size_t irIndex, nanojit::LIns* ins) {
        // printf("map[%lu] = %p\n", index, ins);
        m_IRToLInsMapping[irIndex] = ins;
    }
    nanojit::LIns* getMapping(size_t irIndex) {
        // printf("= map[%lu]\n", index);
        return m_IRToLInsMapping[irIndex];
    }
    std::vector<nanojit::LIns*> m_IRToLInsMapping;

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
