#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJIT.h"

#include "ESGraph.h"
#include "ESJITFrontend.h"
#include "ESJITMiddleend.h"
#include "ESJITBackend.h"

namespace escargot {

class CodeBlock;

namespace ESJIT {

void ESJITFunction::compile()
{
    unsigned long time1 = ESVMInstance::currentInstance()->tickCount();
    m_graph = generateIRFromByteCode(m_codeBlock);
    unsigned long time2 = ESVMInstance::currentInstance()->tickCount();
    optimizeIR(m_graph);
    unsigned long time3 = ESVMInstance::currentInstance()->tickCount();
    m_native = generateNativeFromIR(m_graph);
    unsigned long time4 = ESVMInstance::currentInstance()->tickCount();

    printf("JIT Compilation Took %lfms, %lfms, %lfms each for FE, ME, BE\n",
            (time2-time1)/1000.0, (time3-time2)/1000.0, (time4-time3)/1000.0);
}

void ESJITFunction::finalize()
{
    delete m_graph;
}

JITFunction JITCompile(CodeBlock* codeBlock)
{
    ESJITFunction jitFunction(codeBlock);
    jitFunction.compile();
    jitFunction.finalize();
    return jitFunction.native();
}

}}
#endif
