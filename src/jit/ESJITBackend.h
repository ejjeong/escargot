#ifndef ESJITBackend_h
#define ESJITBackend_h

#ifdef ENABLE_ESJIT

#include "ESIRType.h"
#include "nanojit.h"

namespace escargot {

class ESVMInstance;

namespace ESJIT {

class ESGraph;
class ESIR;

typedef ESValueInDouble (*JITFunction)(ESVMInstance*);

class NativeGenerator {
public:
    NativeGenerator(ESGraph* graph);
    ~NativeGenerator();
    bool nanojitCodegen(ESVMInstance* instance);
    JITFunction nativeCodegen();
    nanojit::LIns* nanojitCodegen(ESIR* ir);

private:
    void setTmpMapping(size_t irIndex, nanojit::LIns* ins)
    {
        // printf("tmpMap[%lu] = %p\n", irIndex, ins);
        ASSERT(irIndex < m_tmpToLInsMapping.size());
        ASSERT(ins);
        m_tmpToLInsMapping[irIndex] = ins;
    }

    nanojit::LIns* getTmpMapping(size_t irIndex)
    {
        // printf("= tmpMap[%lu]\n", irIndex);
        ASSERT(irIndex < m_tmpToLInsMapping.size());
        ASSERT(m_tmpToLInsMapping[irIndex]);
        return m_tmpToLInsMapping[irIndex];
    }
    nanojit::LIns* generateOSRExit(size_t currentByteCodeIndex);
    nanojit::LIns* generateTypeCheck(nanojit::LIns* in, Type type, size_t currentByteCodeIndex);
    nanojit::LIns* boxESValue(nanojit::LIns* value, Type type);
    nanojit::LIns* unboxESValue(nanojit::LIns* value, Type type);
#ifndef ESCARGOT_64
    nanojit::LIns* boxESValueFromTagAndPayload(nanojit::LIns* tag, nanojit::LIns* payload);
    nanojit::LIns* getTagFromESValue(nanojit::LIns* boxedValue);
    nanojit::LIns* getPayloadFromESValue(nanojit::LIns* boxedValue);
#endif

    nanojit::LIns* getDoubleDynamic(nanojit::LIns* in, Type type);
    nanojit::LIns* getInt32Dynamic(nanojit::LIns* in, Type type);

    nanojit::LIns* getOffsetAsPointer(nanojit::LIns* in);

    ESGraph* m_graph;
    std::vector<nanojit::LIns*, CustomAllocator<nanojit::LIns*> > m_tmpToLInsMapping;

    nanojit::LogControl m_lc;
    nanojit::Config m_config;
    nanojit::Allocator* m_alloc;
    nanojit::CodeAlloc* m_codeAlloc;
    nanojit::Assembler* m_assm;
    nanojit::LirBuffer* m_buf;
    nanojit::Fragment* m_f;
    nanojit::LirWriter* m_out;

    nanojit::LIns* m_zeroD;
    nanojit::LIns* m_zeroP;
    nanojit::LIns* m_oneI;
    nanojit::LIns* m_zeroI;
    nanojit::LIns* m_true;
    nanojit::LIns* m_false;
    nanojit::LIns* m_undefinedE;
    nanojit::LIns* m_nullE;
    nanojit::LIns* m_emptyE;
    nanojit::LIns* m_thisValueP;
    nanojit::LIns* m_instanceP;
    nanojit::LIns* m_contextP;
    nanojit::LIns* m_globalObjectP;
    nanojit::LIns* m_cachedDeclarativeEnvironmentRecordESValueP;
#ifdef ESCARGOT_64
    nanojit::LIns* m_tagMaskQ;
    nanojit::LIns* m_booleanTagQ;
    nanojit::LIns* m_booleanTagComplementQ;
    nanojit::LIns* m_intTagQ;
    nanojit::LIns* m_intTagComplementQ;
    nanojit::LIns* m_doubleEncodeOffsetQ;
    nanojit::LIns* m_emptyD;
    nanojit::LIns* m_zeroQ;
#else
    nanojit::LIns* m_int32TagI;
    nanojit::LIns* m_booleanTagI;
    nanojit::LIns* m_nullTagI;
    nanojit::LIns* m_undefinedTagI;
    nanojit::LIns* m_pointerTagI;
    nanojit::LIns* m_emptyValueTagI;
    nanojit::LIns* m_deletedValueTagI;
    nanojit::LIns* m_lowestTagI;
#endif

};

JITFunction generateNativeFromIR(ESGraph* graph, ESVMInstance* instance);

JITFunction addDouble();
int nanoJITTest();

}}
#endif
#endif
