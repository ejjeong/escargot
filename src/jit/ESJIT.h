#ifndef ESJIT_h
#define ESJIT_h

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
        : m_codeBlock(codeBlock), m_ir(nullptr), m_native(nullptr) { }

    void compile();
    void finalize();

    CodeBlock* codeBlock() { return m_codeBlock; }
    ESGraph* ir() { return m_ir; }
    JITFunction native() { return m_native; }

private:
    CodeBlock* m_codeBlock;
    ESGraph* m_ir;
    JITFunction m_native;
};

JITFunction JITCompile(CodeBlock* codeBlock);

}

unsigned long getLongTickCount();

}
#endif
