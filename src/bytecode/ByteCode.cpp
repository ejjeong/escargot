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
    m_cachedJITFunction = nullptr;
    m_executeCount = 0;
    m_jitThreshold = ESVMInstance::currentInstance()->m_jitThreshold;
#endif
    if(!isBuiltInFunction) {
        ESVMInstance::currentInstance()->globalObject()->registerCodeBlock(this);
    }
}

CodeBlock::~CodeBlock()
{
    if(!m_isBuiltInFunction)
        ESVMInstance::currentInstance()->globalObject()->unregisterCodeBlock(this);
}

void CodeBlock::pushCodeFillExtraData(ByteCode* code, ByteCodeExtraData* data, ByteCodeGenerateContext& context)
{
    Opcode op = (Opcode)(size_t)code->m_opcodeInAddress;
    data->m_codePosition = m_code.size();
    data->m_baseRegisterIndex = context.m_baseRegisterCount;
    data->m_registerIncrementCount = pushCountFromOpcode(code, op);
    data->m_registerDecrementCount = popCountFromOpcode(code, op);
    context.m_baseRegisterCount = context.m_baseRegisterCount + data->m_registerIncrementCount - data->m_registerDecrementCount;
    ASSERT(context.m_baseRegisterCount>=0);

#ifdef ENABLE_ESJIT
    if(op == AllocPhiOpcode) {
        data->m_targetIndex0 = context.m_currentSSARegisterCount++;
    } else if(op == StorePhiOpcode) {
        data->m_sourceIndexes.push_back(((StorePhi *)code)->m_allocIndex);
        data->m_sourceIndexes.push_back(context.m_currentSSARegisterCount - 1);
        data->m_targetIndex0 = context.m_currentSSARegisterCount++;
    } else if(op == LoadPhiOpcode) {
        data->m_sourceIndexes.push_back(((LoadPhi *)code)->m_allocIndex);
        data->m_sourceIndexes.push_back(((LoadPhi *)code)->m_srcIndex0);
        data->m_sourceIndexes.push_back(((LoadPhi *)code)->m_srcIndex1);
        data->m_targetIndex0 = context.m_currentSSARegisterCount++;
    } else if(op == PushIntoTempStackOpcode) {
        data->m_targetIndex0 = context.m_currentSSARegisterCount++;
        data->m_sourceIndexes.push_back(context.m_ssaComputeStack.back());
        context.m_ssaComputeStack.pop_back();
    } else if(op == PopFromTempStackOpcode) {
        int val = -1;
        for(unsigned i = m_extraData.size() - 1; ; i --) {
            if(m_extraData[i].m_codePosition == ((PopFromTempStack *)code)->m_pushCodePosition) {
                val = m_extraData[i].m_targetIndex0;
                data->m_sourceIndexes.push_back(val);
                break;
            }
        }
        ASSERT(val != -1);
        data->m_targetIndex0 = context.m_currentSSARegisterCount++;
        context.m_ssaComputeStack.push_back(val);
    } else {
        // normal path
        if(op == InitObjectOpcode) {
            // peek, pop both are exist case
            int peekCount = peekCountFromOpcode(code, op);
            ASSERT(peekCount == 1);
            for(int i = 0; i < data->m_registerDecrementCount ; i ++) {
                int c = context.m_ssaComputeStack.back();
                context.m_ssaComputeStack.pop_back();
                data->m_sourceIndexes.push_back(c);
            }

            data->m_sourceIndexes.push_back(context.m_ssaComputeStack.back());
        } else {
            // normal path
            int peekCount = peekCountFromOpcode(code, op);
            ASSERT(!peekCount || !data->m_registerDecrementCount);
            auto iter = context.m_ssaComputeStack.end();
            for(int i = 0; i < peekCount ; i ++) {
                iter --;
                int c = *iter;
                data->m_sourceIndexes.push_back(c);
            }

            for(int i = 0; i < data->m_registerDecrementCount ; i ++) {
                int c = context.m_ssaComputeStack.back();
                context.m_ssaComputeStack.pop_back();
                data->m_sourceIndexes.push_back(c);
            }
        }

        std::reverse(data->m_sourceIndexes.begin(), data->m_sourceIndexes.end());

        if(data->m_registerIncrementCount == 0) {
        } else if(data->m_registerIncrementCount == 1) {
            int c = context.m_currentSSARegisterCount++;
            context.m_ssaComputeStack.push_back(c);
            data->m_targetIndex0 = c;
        } else {
            ASSERT(data->m_registerIncrementCount == 2);
            int c = context.m_currentSSARegisterCount++;
            context.m_ssaComputeStack.push_back(c);
            data->m_targetIndex0 = c;
            c = context.m_currentSSARegisterCount++;
            context.m_ssaComputeStack.push_back(c);
            data->m_targetIndex1 = c;
        }
    }

    bool haveToProfile = false;
    bool canJIT = false;

#define FETCH_DATA_BYTE_CODE(code, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
    case code##Opcode: \
        haveToProfile = hasProfileData; \
        canJIT = JITSupported; \
    break;
    switch(op) {
        FOR_EACH_BYTECODE_OP(FETCH_DATA_BYTE_CODE);
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }

#ifdef NDEBUG
    if(!canJIT) {
        m_dontJIT = true;
    }
#endif

    if(haveToProfile) {
        m_byteCodeIndexesHaveToProfile.push_back(m_extraData.size());
    }
#endif
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
    // unsigned long start = ESVMInstance::tickCount();
    node->generateStatementByteCode(block, context);
    // unsigned long end = ESVMInstance::tickCount();
    // printf("generate code takes %lfms\n",(end-start)/1000.0);
#ifndef NDEBUG
    if(ESVMInstance::currentInstance()->m_dumpByteCode) {
        char* code = block->m_code.data();
        ByteCode* currentCode = (ByteCode *)(&code[0]);
        if(currentCode->m_orgOpcode != ExecuteNativeFunctionOpcode) {
            dumpBytecode(block);
        }
    }
#endif

#ifdef ENABLE_ESJIT
    // Fill temp register size for future
    block->m_tempRegisterSize = context.m_currentSSARegisterCount;
    context.cleanupSSARegisterCount();
#endif
    return block;
}

unsigned char popCountFromOpcode(ByteCode* code, Opcode opcode)
{
    if(opcode == CallFunctionOpcode) {
        CallFunction* c = (CallFunction*)code;
        return c->m_argmentCount + 1/* function */;
    } if(opcode == CallFunctionWithReceiverOpcode) {
        CallFunctionWithReceiver* c = (CallFunctionWithReceiver*)code;
        return c->m_argmentCount + 1/* receiver */ + 1/* function */;
    } else if(opcode == CallEvalFunctionOpcode) {
        CallEvalFunction* c = (CallEvalFunction*)code;
        return c->m_argmentCount;
    } else if(opcode == NewFunctionCallOpcode) {
        NewFunctionCall* c = (NewFunctionCall*)code;
        return c->m_argmentCount + 1/* function */;
    }
#define FETCH_POP_COUNT_BYTE_CODE(code, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
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
#define FETCH_PUSH_COUNT_BYTE_CODE(code, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
    case code##Opcode: \
        ASSERT(pushCount != -1); \
        return pushCount;
    switch(opcode) {
        FOR_EACH_BYTECODE_OP(FETCH_PUSH_COUNT_BYTE_CODE);
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

unsigned char peekCountFromOpcode(ByteCode* code, Opcode opcode)
{
#define FETCH_PEEK_COUNT_BYTE_CODE(code, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
    case code##Opcode: \
        ASSERT(peekCount != -1); \
        return peekCount;
    switch(opcode) {
        FOR_EACH_BYTECODE_OP(FETCH_PEEK_COUNT_BYTE_CODE);
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

#ifndef NDEBUG

void dumpBytecode(CodeBlock* codeBlock)
{
    printf("dumpBytecode...>>>>>>>>>>>>>>>>>>>>>>\n");
    printf("function %s (codeBlock %p)\n", codeBlock->m_nonAtomicId ? (codeBlock->m_nonAtomicId->utf8Data()):"(anonymous)", codeBlock);
    size_t idx = 0;
    size_t bytecodeCounter = 0;
#ifdef ENABLE_ESJIT
    size_t callInfoIndex = 0;
#endif
    char* code = codeBlock->m_code.data();
    char* end = &codeBlock->m_code.data()[codeBlock->m_code.size()];
    while(&code[idx] < end) {
        ByteCode* currentCode = (ByteCode *)(&code[idx]);
        unsigned currentCount = bytecodeCounter;
        ByteCodeExtraData* ex = &codeBlock->m_extraData[currentCount];
        if(currentCode->m_node)
            printf("%u\t\t%p\t(nodeinfo %d)\t\t",(unsigned)idx, currentCode, (int)currentCode->m_node->sourceLocation().m_lineNumber);
        else
            printf("%u\t\t%p\t(nodeinfo null)\t\t",(unsigned)idx, currentCode);

        printf("IdxInfo[%d,+%d,-%d]\t", ex->m_baseRegisterIndex, ex->m_registerIncrementCount, ex->m_registerDecrementCount);
#ifdef ENABLE_ESJIT
        printf("ssa->[");

        if(ex->m_targetIndex0 != -1) {
            printf("t: %d,", ex->m_targetIndex0);
        }

        if(ex->m_targetIndex1 != -1) {
            printf("t2: %d,", ex->m_targetIndex1);
        }

        for(int i = 0; i < ex->m_sourceIndexes.size() ; i ++) {
            printf("s: %d,", ex->m_sourceIndexes[i]);
        }
        printf("]");
#endif
        printf("\t");

        Opcode opcode = codeBlock->m_extraData[bytecodeCounter].m_opcode;
        bytecodeCounter++;
        ASSERT(opcode == currentCode->m_orgOpcode);

        switch(opcode) {
#define DUMP_BYTE_CODE(code, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
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
    }
    printf("dumpBytecode...<<<<<<<<<<<<<<<<<<<<<<\n");
}

void dumpUnsupported(CodeBlock* block)
{
    const char* functionName = block->m_nonAtomicId ? (block->m_nonAtomicId->utf8Data()):"(anonymous)";
    printf("Unsupported opcodes for function %s (codeBlock %p)\n", functionName, block);
    size_t idx = 0;
    size_t bytecodeCounter = 0;
    char* code = block->m_code.data();
    char* end = &block->m_code.data()[block->m_code.size()];
    std::map<std::string, size_t> names;
    while(&code[idx] < end) {
        Opcode opcode = block->m_extraData[bytecodeCounter].m_opcode;
        switch(opcode) {
#define DECLARE_EXECUTE_NEXTCODE(opcode, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
        case opcode##Opcode: \
            if (!JITSupported) { \
                auto result = names.insert(std::pair<std::string, size_t>(std::string(#opcode), 1)); \
                if (!result.second) \
                    names[std::string(#opcode)]++; \
            } \
            idx += sizeof (opcode); \
            bytecodeCounter++; \
            break;
            FOR_EACH_BYTECODE_OP(DECLARE_EXECUTE_NEXTCODE);
#undef DECLARE_EXECUTE_NEXTCODE
        case OpcodeKindEnd:
            break;
        }
    }
    for (auto it=names.begin(); it!=names.end(); ++it)
        std::cout << it->first << "(" << it->second << ") ";
    printf("\n");
}
#endif
}


