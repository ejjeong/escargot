#ifndef ESJITBackend_h
#define ESJITBackend_h

#ifdef ENABLE_ESJIT

#include "ESIRType.h"
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
    void setTmpMapping(size_t irIndex, nanojit::LIns* ins) {
        // printf("tmpMap[%lu] = %p\n", irIndex, ins);
        m_tmpToLInsMapping[irIndex] = ins;
    }
    nanojit::LIns* getTmpMapping(size_t irIndex) {
        // printf("= tmpMap[%lu]\n", irIndex);
        return m_tmpToLInsMapping[irIndex];
    }
    void setVarMapping(size_t irIndex, nanojit::LIns* ins) {
        // printf("varMap[%lu] = %p\n", irIndex, ins);
        m_varToLInsMapping[irIndex] = ins;
    }
    nanojit::LIns* getVarMapping(size_t irIndex) {
        // printf("= varMap[%lu]\n", irIndex);
        return m_varToLInsMapping[irIndex];
    }
    nanojit::LIns* generateOSRExit(size_t currentByteCodeIndex);
    nanojit::LIns* generateTypeCheck(nanojit::LIns* in, Type type, size_t currentByteCodeIndex);
    nanojit::LIns* boxESValue(nanojit::LIns* value, Type type);
    nanojit::LIns* unboxESValue(nanojit::LIns* value, Type type);

    ESGraph* m_graph;
    std::vector<nanojit::LIns*, gc_allocator<nanojit::LIns*> > m_tmpToLInsMapping;
    std::vector<nanojit::LIns*, gc_allocator<nanojit::LIns*> > m_varToLInsMapping;

    nanojit::LIns* m_stackPtr;
    nanojit::LIns* m_instance;
    nanojit::LIns* m_context;
    nanojit::LIns* m_globalObject;

    nanojit::LogControl m_lc;
    nanojit::Config m_config;
    nanojit::Allocator* m_alloc;
    nanojit::CodeAlloc* m_codeAlloc;
    nanojit::Assembler* m_assm;
    nanojit::LirBuffer* m_buf;
    nanojit::Fragment* m_f;
    nanojit::LirBufWriter m_out;

    nanojit::LIns* m_intTagQ;
    nanojit::LIns* m_intTagComplementQ;
    nanojit::LIns* m_zeroQ;
    nanojit::LIns* m_zeroP;
    nanojit::LIns* m_oneI;
    nanojit::LIns* m_zeroI;
    nanojit::LIns* m_true;
    nanojit::LIns* m_false;
};

JITFunction generateNativeFromIR(ESGraph* graph);

JITFunction addDouble();
int nanoJITTest();

}}
#endif
#endif
