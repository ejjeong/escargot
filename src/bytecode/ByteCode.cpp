#include "Escargot.h"
#include "bytecode/ByteCode.h"

#include "ByteCodeOperations.h"

namespace escargot {

CodeBlock::CodeBlock()
{
    m_needsActivation = false;
    m_isBuiltInFunction = false;
    m_isStrict = false;
#ifdef ENABLE_ESJIT
    m_executeCount = 0;
#endif

    ESVMInstance::currentInstance()->globalObject()->registerCodeBlock(this);
}

CodeBlock::~CodeBlock()
{
    ESVMInstance::currentInstance()->globalObject()->unregisterCodeBlock(this);
}

ByteCode::ByteCode(Opcode code) {
    m_opcode = (ESVMInstance::currentInstance()->opcodeTable())->m_table[(unsigned)code];
#ifndef NDEBUG
    m_orgOpcode = code;
    m_node = nullptr;
#endif
}

CodeBlock* generateByteCode(Node* node)
{
    CodeBlock* block = CodeBlock::create();

    ByteCodeGenerateContext context;
    //unsigned long start = ESVMInstance::tickCount();
    node->generateStatementByteCode(block, context);
    //unsigned long end = ESVMInstance::tickCount();
    //printf("generate code takes %lfms\n",(end-start)/1000.0);
#ifndef NDEBUG
    if(ESVMInstance::currentInstance()->m_dumpByteCode) {
        char* code = block->m_code.data();
        ByteCode* currentCode = (ByteCode *)(&code[0]);
        if(currentCode->m_orgOpcode != ExecuteNativeFunctionOpcode) {
            dumpBytecode(block);
        }
    }
#endif
    return block;
}

#ifndef NDEBUG

void dumpBytecode(CodeBlock* codeBlock)
{
    printf("dumpBytecode...>>>>>>>>>>>>>>>>>>>>>>\n");
    printf("function %s\n", codeBlock->m_nonAtomicId ? (codeBlock->m_nonAtomicId->utf8Data()):"(anonymous)");
    size_t idx = 0;
#ifdef ENABLE_ESJIT
    size_t bytecodeCounter = 0;
    size_t callInfoIndex = 0;
#endif
    char* code = codeBlock->m_code.data();
    while(idx < codeBlock->m_code.size()) {
        ByteCode* currentCode = (ByteCode *)(&code[idx]);
        if(currentCode->m_node)
            printf("%u\t\t%p\t(nodeinfo %d)\t\t\t",(unsigned)idx, currentCode, (int)currentCode->m_node->sourceLocation().m_lineNumber);
        else
            printf("%u\t\t%p\t(nodeinfo null)\t\t\t",(unsigned)idx, currentCode);

        Opcode opcode = opcodeFromAddress(currentCode->m_opcode);

#ifdef ENABLE_ESJIT
        if (opcode == CallFunctionOpcode) {
            int calleeIndex = codeBlock->m_functionCallInfos[callInfoIndex++];
            int receiverIndex = codeBlock->m_functionCallInfos[callInfoIndex++];
            int argumentCount = codeBlock->m_functionCallInfos[callInfoIndex++];
            printf("[%3d,%3d,%3d", calleeIndex, receiverIndex, argumentCount);
            for (int i=0; i<argumentCount; i++)
                printf(",%3d", codeBlock->m_functionCallInfos[callInfoIndex++]);
            printf("] ");
        }
        switch(opcode) {
#define DUMP_BYTE_CODE(code) \
        case code##Opcode:\
        codeBlock->getSSAIndex(bytecodeCounter)->dump(); \
        currentCode->dump(); \
        idx += sizeof (code); \
        bytecodeCounter++; \
        continue;
        FOR_EACH_BYTECODE_OP(DUMP_BYTE_CODE)
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        };
#else // ENABLE_ESJIT
        switch(opcode) {
#define DUMP_BYTE_CODE(code) \
        case code##Opcode:\
        currentCode->dump(); \
        idx += sizeof (code); \
        continue;
        FOR_EACH_BYTECODE_OP(DUMP_BYTE_CODE)
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        };
#endif // ENABLE_ESJIT
    }
    printf("dumpBytecode...<<<<<<<<<<<<<<<<<<<<<<\n");
}

#endif
}
