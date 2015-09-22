#ifndef ESJIT_h
#define ESJIT_h

#ifdef ENABLE_ESJIT

#include "runtime/ESValue.h"

namespace escargot {

class CodeBlock;
class ESVMInstance;

namespace ESJIT {

class ESGraph;

typedef ESValue (*JITFunction)(ESVMInstance*);

class ESJITFunction {
public:
    ESJITFunction(CodeBlock* codeBlock)
        : m_codeBlock(codeBlock), m_graph(nullptr), m_native(nullptr) { }

    void compile();
    void finalize();

    CodeBlock* codeBlock() { return m_codeBlock; }
    ESGraph* ir() { return m_graph; }
    JITFunction native() { return m_native; }

private:
    CodeBlock* m_codeBlock;
    ESGraph* m_graph;
    JITFunction m_native;
};

JITFunction JITCompile(CodeBlock* codeBlock);

}

unsigned long getLongTickCount();

}
#endif
#endif
