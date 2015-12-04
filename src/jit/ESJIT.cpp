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

ESJITCompiler::ESJITCompiler(CodeBlock* codeBlock)
    : m_codeBlock(codeBlock), m_graph(nullptr), m_native(nullptr)
{
#ifdef ESCARGOT_PROFILE
    m_startTime = ESVMInstance::currentInstance()->tickCount();
#endif
    GC_disable();
}

ESJITCompiler::~ESJITCompiler()
{
    ESJITAllocator::freeAll();
    GC_enable();

#ifdef ESCARGOT_PROFILE
    unsigned long endTime = ESVMInstance::currentInstance()->tickCount();
    if (ESVMInstance::currentInstance()->m_profile)
        printf("JIT Compilation Took %lfms\n", (endTime - m_startTime) / 1000.0);
#endif
}

bool ESJITCompiler::compile(ESVMInstance* instance)
{
    if (!(m_graph = generateIRFromByteCode(m_codeBlock)))
        return false;
    if (!optimizeIR(m_graph))
        return false;
    if (!(m_native = generateNativeFromIR(m_graph, instance)))
        return false;
    return true;
}

JITFunction JITCompile(CodeBlock* codeBlock, ESVMInstance* instance)
{
#ifdef NDEBUG
    codeBlock->fillExtraData();
#endif
    ESJITCompiler jitFunction(codeBlock);
    if (!jitFunction.compile(instance))
        return nullptr;
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
