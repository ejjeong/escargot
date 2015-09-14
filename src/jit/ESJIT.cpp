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
    m_ir = generateIRFromByteCode(m_codeBlock);
#ifndef NDEBUG
    m_ir->dump(std::cout);
#endif
    optimizeIR(m_ir);
    m_native = generateNativeFromIR(m_ir);
}

void ESJITFunction::finalize()
{
    delete m_ir;
}

JITFunction JITCompile(CodeBlock* codeBlock)
{
    ESJITFunction jitFunction(codeBlock);
    jitFunction.compile();
    jitFunction.finalize();
    return jitFunction.native();
}

}}
