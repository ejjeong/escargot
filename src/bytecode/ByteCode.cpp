#include "Escargot.h"
#include "bytecode/ByteCode.h"

namespace escargot {

CodeBlock::CodeBlock(bool isBuiltInFunction)
{
    m_needsActivation = false;
    m_isBuiltInFunction = isBuiltInFunction;
    m_isStrict = false;
    m_isFunctionExpression = false;
    m_requiredStackSizeInESValueSize = 0;
#ifdef ENABLE_ESJIT
    m_executeCount = 0;
    m_threshold = 1;
#endif
    if(!isBuiltInFunction)
        ESVMInstance::currentInstance()->globalObject()->registerCodeBlock(this);
}

CodeBlock::~CodeBlock()
{
    if(!m_isBuiltInFunction)
        ESVMInstance::currentInstance()->globalObject()->unregisterCodeBlock(this);
}

void CodeBlock::pushCodeFillExtraData(ByteCode* code, ByteCodeExtraData* data, ByteCodeGenerateContext& context)
{
    Opcode op = (Opcode)(size_t)code->m_opcodeInAddress;
    data->m_baseRegisterIndex = context.m_baseRegisterCount;
    data->m_registerIncrementCount = pushCountFromOpcode(code, op);
    data->m_registerDecrementCount = popCountFromOpcode(code, op);
    context.m_baseRegisterCount = context.m_baseRegisterCount + data->m_registerIncrementCount - data->m_registerDecrementCount;
    ASSERT(context.m_baseRegisterCount>=0);
}

ByteCode::ByteCode(Opcode code) {
    m_opcodeInAddress = (void *)(size_t)code;
#ifndef NDEBUG
    m_orgOpcode = code;
    m_node = nullptr;
#endif
}

void ByteCode::assignOpcodeInAddress()
{
    Opcode op = (Opcode)(size_t)m_opcodeInAddress;
    m_opcodeInAddress = (ESVMInstance::currentInstance()->opcodeTable())->m_table[op];
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

unsigned char popCountFromOpcode(ByteCode* code, Opcode opcode)
{
    if(opcode == CallFunctionOpcode) {
        CallFunction* c = (CallFunction*)code;
        return c->m_argmentCount + 1/* receiver */ + 1/* function */;
    } else if(opcode == CallEvalFunctionOpcode) {
        CallEvalFunction* c = (CallEvalFunction*)code;
        return c->m_argmentCount;
    } else if(opcode == NewFunctionCallOpcode) {
        NewFunctionCall* c = (NewFunctionCall*)code;
        return c->m_argmentCount + 1/* function */;
    }
#define FETCH_POP_COUNT_BYTE_CODE(code, pushCount, popCount) \
    case code##Opcode: \
        ASSERT(popCount != -1); \
        return popCount;
    switch(opcode) {
    FOR_EACH_BYTECODE_OP(FETCH_POP_COUNT_BYTE_CODE);
    default:
            RELEASE_ASSERT_NOT_REACHED();
    }
}

unsigned char pushCountFromOpcode(ByteCode* code, Opcode opcode)
{
    if(opcode == CreateFunctionOpcode) {
        if(((CreateFunction *)code)->m_codeBlock->m_isFunctionExpression) {
            return 1;
        } else
            return 0;
    }
#define FETCH_PUSH_COUNT_BYTE_CODE(code, pushCount, popCount) \
case code##Opcode: \
    ASSERT(pushCount != -1); \
    return pushCount;
    switch(opcode) {
    FOR_EACH_BYTECODE_OP(FETCH_PUSH_COUNT_BYTE_CODE);
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
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
    size_t byteCodeIndex = 0;
    char* end = &codeBlock->m_code.data()[codeBlock->m_code.size()];
    while(&code[idx] < end) {
        ByteCode* currentCode = (ByteCode *)(&code[idx]);
        ByteCodeExtraData* ex = &codeBlock->m_extraData[byteCodeIndex++];
        if(currentCode->m_node)
            printf("%u\t\t%p\t(nodeinfo %d)\t\t",(unsigned)idx, currentCode, (int)currentCode->m_node->sourceLocation().m_lineNumber);
        else
            printf("%u\t\t%p\t(nodeinfo null)\t\t",(unsigned)idx, currentCode);

        printf("regIndex[%d,+%d,-%d]\t\t", ex->m_baseRegisterIndex, ex->m_registerIncrementCount, ex->m_registerDecrementCount);

        Opcode opcode = codeBlock->m_extraData[bytecodeCounter].m_opcode;

#ifdef ENABLE_ESJIT
        if (opcode == CallFunctionOpcode || opcode == NewFunctionCallOpcode) {
            int calleeIndex = codeBlock->m_functionCallInfos[callInfoIndex++];
            int receiverIndex = codeBlock->m_functionCallInfos[callInfoIndex++];
            int argumentCount = codeBlock->m_functionCallInfos[callInfoIndex++];
            printf("[%3d,%3d,%3d", calleeIndex, receiverIndex, argumentCount);
            for (int i=0; i<argumentCount; i++)
                printf(",%3d", codeBlock->m_functionCallInfos[callInfoIndex++]);
            printf("] ");
        }
        switch(opcode) {
#define DUMP_BYTE_CODE(code, pushCount, popCount) \
        case code##Opcode:\
        codeBlock->getSSAIndex(bytecodeCounter)->dump(); \
        currentCode->dump(); \
        idx += sizeof (code); \
        bytecodeCounter++; \
        continue;
        FOR_EACH_BYTECODE_OP(DUMP_BYTE_CODE)
#undef  DUMP_BYTE_CODE
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        };
#else // ENABLE_ESJIT
        switch(opcode) {
#define DUMP_BYTE_CODE(code, pushCount, popCount) \
        case code##Opcode:\
        currentCode->dump(); \
        idx += sizeof (code); \
        continue;
        FOR_EACH_BYTECODE_OP(DUMP_BYTE_CODE)
#undef  DUMP_BYTE_CODE
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
