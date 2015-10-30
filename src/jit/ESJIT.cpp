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

std::vector<ESJITAllocatorMemoryFragment> ESJITAllocator::m_allocatedMemorys;

void ESJITAllocator::allocSlow()
{
    const unsigned s_fragmentBufferSize = 10240;
    ESJITAllocatorMemoryFragment f;
    f.m_buffer = malloc(s_fragmentBufferSize);
    f.m_currentUsage = 0;
    f.m_totalSize = s_fragmentBufferSize;
    m_allocatedMemorys.push_back(f);
}

void ESJITAllocator::freeAll()
{
    for(unsigned i = 0 ; i < m_allocatedMemorys.size() ; i ++) {
        free(m_allocatedMemorys[i].m_buffer);
    }
    m_allocatedMemorys.clear();
}

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

JITFunction JITCompile(CodeBlock* codeBlock, ESVMInstance* instance)
{
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
