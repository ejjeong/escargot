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

bool ESJITCompiler::compile(ESVMInstance* instance)
{
    unsigned long time1 = ESVMInstance::currentInstance()->tickCount();
    if (!(m_graph = generateIRFromByteCode(m_codeBlock)))
        return false;

    unsigned long time2 = ESVMInstance::currentInstance()->tickCount();
    if (!optimizeIR(m_graph))
        return false;

    unsigned long time3 = ESVMInstance::currentInstance()->tickCount();
    if (!(m_native = generateNativeFromIR(m_graph, instance)))
        return false;
    unsigned long time4 = ESVMInstance::currentInstance()->tickCount();

    if (ESVMInstance::currentInstance()->m_profile)
        printf("JIT Compilation Took %lfms, %lfms, %lfms each for FE, ME, BE\n",
                (time2-time1)/1000.0, (time3-time2)/1000.0, (time4-time3)/1000.0);

    return true;
}

void ESJITCompiler::finalize()
{
    if (!m_graph) return;
    for (size_t i = 0; i < m_graph->basicBlockSize(); i++) {
        ESBasicBlock* block = m_graph->basicBlock(i);
        for (size_t j = 0; j < block->instructionSize(); j++) {
            ESIR* ir = block->instruction(j);
            free(ir);
        }
        free(block);
    }

    delete m_graph;
}

JITFunction JITCompile(CodeBlock* codeBlock, ESVMInstance* instance)
{
    ESJITCompiler jitFunction(codeBlock);
    GC_disable();
    bool res = jitFunction.compile(instance);
    jitFunction.finalize();
    GC_enable();
    if (!res) return nullptr;
    return jitFunction.native();
}

void logVerboseJIT(const char* format...)
{
#ifndef NDEBUG
    if (ESVMInstance::currentInstance()->m_verboseJIT) {
        va_list argList;
        va_start(argList, format);
        vprintf(format, argList);
        va_end(argList);
    }
#else
    // do nothing
#endif
}

}}
#endif
