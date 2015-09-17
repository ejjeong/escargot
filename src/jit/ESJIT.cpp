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
    m_graph = generateIRFromByteCode(m_codeBlock);
#ifndef NDEBUG
    m_graph->dump(std::cout);
#endif
    optimizeIR(m_graph);
    m_native = generateNativeFromIR(m_graph);
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
