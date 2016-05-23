#include "Escargot.h"
#include "bytecode/ByteCode.h"
#include "ast/AST.h"
#ifdef ENABLE_ESJIT
#include "nanojit.h"
#endif

namespace escargot {

CodeBlock::CodeBlock(ExecutableType type, size_t roughCodeBlockSizeInWordSize, bool isBuiltInFunction)
{
    m_type = type;
    m_ast = NULL;
    if (roughCodeBlockSizeInWordSize)
        m_code.reserve(roughCodeBlockSizeInWordSize * sizeof(size_t));
    m_stackAllocatedIdentifiersCount = 0;
    m_isBuiltInFunction = isBuiltInFunction;
    m_isStrict = false;
    m_isFunctionExpression = false;
    m_requiredStackSizeInESValueSize = 0;
    m_argumentCount = 0;
    m_isCached = false;
    m_hasCode = false;
    m_needsHeapAllocatedExecutionContext = false;
    m_needsComplexParameterCopy = false;
    m_needsToPrepareGenerateArgumentsObject = false;
    m_hasArgumentsBinding = false;
    m_isFunctionExpressionNameHeapAllocated = false;
    m_needsActivation = false;
    m_functionExpressionNameIndex = SIZE_MAX;
    m_forceDenyStrictMode = false;
#ifdef ENABLE_ESJIT
    m_cachedJITFunction = nullptr;
    m_executeCount = 0;
    m_jitThreshold = ESVMInstance::currentInstance()->m_jitThreshold;
    m_dontJIT = false;
    m_recursionDepth = 0;
    m_codeAlloc = nullptr;
    m_nanoJITDataAllocator = nullptr;
#endif

    ESVMInstance* instance = ESVMInstance::currentInstance();
    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        ((CodeBlock *)obj)->finalize((ESVMInstance*)cd);
    }, instance, NULL, NULL);
    instance->globalObject()->registerCodeBlock(this);
}

void CodeBlock::finalize(ESVMInstance* instance)
{
    GC_REGISTER_FINALIZER_NO_ORDER(this, NULL, NULL, NULL, NULL);

    instance->globalObject()->unregisterCodeBlock(this);

    m_code.clear();
    m_code.shrink_to_fit();
    RELEASE_ASSERT(!m_code.capacity());
#ifdef ENABLE_ESJIT
    removeJITInfo();
    removeJITCode();
    ASSERT(m_recursionDepth == 0);
#endif
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
    m_extraData.clear();
    m_extraData.shrink_to_fit();
    RELEASE_ASSERT(!m_extraData.capacity());
#endif
}

#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
void CodeBlock::pushCodeFillExtraData(ByteCode* code, ByteCodeExtraData* data, ByteCodeGenerateContext& context, size_t codePosition, size_t bytecodeCount)
#else
void CodeBlock::pushCodeFillExtraData(ByteCode* code, ByteCodeExtraData* data, ByteCodeGenerateContext& context, size_t codePosition)
#endif
{
    Opcode op = (Opcode)(size_t)code->m_opcodeInAddress;
    char registerIncrementCount = pushCountFromOpcode(code, op);
    char registerDecrementCount = popCountFromOpcode(code, op);
    context.m_baseRegisterCount = context.m_baseRegisterCount + registerIncrementCount - registerDecrementCount;
    ASSERT(context.m_baseRegisterCount >= 0);

#ifdef ENABLE_ESJIT
    bool haveToProfile = false;
    bool canJIT = false;

#define FETCH_DATA_BYTE_CODE(code, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
    case code##Opcode: \
        haveToProfile = hasProfileData; \
        canJIT = JITSupported; \
    break;
    switch (op) {
        FOR_EACH_BYTECODE_OP(FETCH_DATA_BYTE_CODE);
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }

#ifdef NDEBUG
    if (!canJIT) {
        m_dontJIT = true;
    }
#endif

    if (haveToProfile) {
        m_byteCodePositionsHaveToProfile.push_back(codePosition);
        m_byteCodeIndexesHaveToProfile.push_back(bytecodeCount);
    }
#endif
}

#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
void CodeBlock::fillExtraData(ByteCode* code, ByteCodeExtraData* data, ExtraDataGenerateContext& context, size_t codePosition, size_t bytecodeCount)
{
    Opcode op = data->m_opcode;
#if defined(ENABLE_ESJIT) && defined(NDEBUG)
    if (data->m_decoupledData == nullptr)
        data->m_decoupledData = new ByteCodeExtraData::DecoupledData();
#endif
    ASSERT(data->m_decoupledData);
    ASSERT(data->m_decoupledData->m_codePosition == SIZE_MAX);
    char registerIncrementCount = pushCountFromOpcode(code, op);
    char registerDecrementCount = popCountFromOpcode(code, op);
    data->m_decoupledData->m_codePosition = codePosition;
    data->m_decoupledData->m_baseRegisterIndex = context.m_baseRegisterCount;
    data->m_decoupledData->m_registerIncrementCount = registerIncrementCount;
    data->m_decoupledData->m_registerDecrementCount = registerDecrementCount;
    context.m_baseRegisterCount = context.m_baseRegisterCount + registerIncrementCount - registerDecrementCount;
    ASSERT(context.m_baseRegisterCount >= 0);

#ifdef ENABLE_ESJIT
    if (op == AllocPhiOpcode) {
        data->m_decoupledData->m_targetIndex0 = context.m_currentSSARegisterCount++;
        context.m_phiIndexToAllocIndexMapping[((AllocPhi *)code)->m_phiIndex] = data->m_decoupledData->m_targetIndex0;
    } else if (op == StorePhiOpcode) {
        size_t allocIndex = context.m_phiIndexToAllocIndexMapping[((StorePhi *)code)->m_phiIndex];
        bool consumeSource = ((StorePhi *)code)->m_consumeSource;
        bool isConsequent = ((StorePhi *)code)->m_isConsequent;
        std::pair<int, int>& storeIndexes = context.m_phiIndexToStoreIndexMapping[((StorePhi *)code)->m_phiIndex];
        data->m_decoupledData->m_sourceIndexes.push_back(allocIndex);
        data->m_decoupledData->m_sourceIndexes.push_back(context.m_currentSSARegisterCount - 1);
        data->m_decoupledData->m_targetIndex0 = context.m_currentSSARegisterCount++;
        if (isConsequent)
            storeIndexes.first = data->m_decoupledData->m_targetIndex0;
        else
            storeIndexes.second = data->m_decoupledData->m_targetIndex0;
        if (consumeSource)
            context.m_ssaComputeStack.pop_back();
    } else if (op == LoadPhiOpcode) {
        size_t allocIndex = context.m_phiIndexToAllocIndexMapping[((LoadPhi *)code)->m_phiIndex];
        size_t srcIndex0 = context.m_phiIndexToStoreIndexMapping[((LoadPhi *)code)->m_phiIndex].first;
        size_t srcIndex1 = context.m_phiIndexToStoreIndexMapping[((LoadPhi *)code)->m_phiIndex].second;
        int targetIndex = context.m_currentSSARegisterCount++;
        data->m_decoupledData->m_sourceIndexes.push_back(allocIndex);
        data->m_decoupledData->m_sourceIndexes.push_back(srcIndex0);
        data->m_decoupledData->m_sourceIndexes.push_back(srcIndex1);
        data->m_decoupledData->m_targetIndex0 = targetIndex;
        context.m_ssaComputeStack.push_back(targetIndex);
    } else if (op == FakePopOpcode) {
        context.m_baseRegisterCount--;
    } else if (op == PushIntoTempStackOpcode) {
        data->m_decoupledData->m_targetIndex0 = context.m_currentSSARegisterCount++;
        data->m_decoupledData->m_sourceIndexes.push_back(context.m_ssaComputeStack.back());
        context.m_ssaComputeStack.pop_back();
    } else if (op == PopFromTempStackOpcode) {
        int val = -1;
        for (unsigned i = bytecodeCount - 1; ; i --) {
            if (m_extraData[i].m_decoupledData->m_codePosition == ((PopFromTempStack *)code)->m_pushCodePosition) {
                val = m_extraData[i].m_decoupledData->m_targetIndex0;
                data->m_decoupledData->m_sourceIndexes.push_back(val);
                break;
            }
        }
        ASSERT(val != -1);
        data->m_decoupledData->m_targetIndex0 = context.m_currentSSARegisterCount++;
        context.m_ssaComputeStack.push_back(val);
    } else {
        // normal path
        if (op == InitObjectOpcode) {
            // peek, pop both are exist case
            int peekCount = peekCountFromOpcode(code, op);
            ASSERT(peekCount == 1);
            for (int i = 0; i < data->m_decoupledData->m_registerDecrementCount ; i ++) {
                int c = context.m_ssaComputeStack.back();
                context.m_ssaComputeStack.pop_back();
                data->m_decoupledData->m_sourceIndexes.push_back(c);
            }

            data->m_decoupledData->m_sourceIndexes.push_back(context.m_ssaComputeStack.back());
        } else {
            // normal path
            int peekCount = peekCountFromOpcode(code, op);
            ASSERT(!peekCount || !data->m_decoupledData->m_registerDecrementCount);
            auto iter = context.m_ssaComputeStack.end();
            for (int i = 0; i < peekCount ; i ++) {
                iter--;
                int c = *iter;
                data->m_decoupledData->m_sourceIndexes.push_back(c);
            }

            for (int i = 0; i < data->m_decoupledData->m_registerDecrementCount ; i ++) {
                int c = context.m_ssaComputeStack.back();
                context.m_ssaComputeStack.pop_back();
                data->m_decoupledData->m_sourceIndexes.push_back(c);
            }
        }

        std::reverse(data->m_decoupledData->m_sourceIndexes.begin(), data->m_decoupledData->m_sourceIndexes.end());

        if (registerIncrementCount == 0) {
        } else if (registerIncrementCount == 1) {
            int c = context.m_currentSSARegisterCount++;
            context.m_ssaComputeStack.push_back(c);
            data->m_decoupledData->m_targetIndex0 = c;
        } else {
            ASSERT(registerIncrementCount == 2);
            int c = context.m_currentSSARegisterCount++;
            context.m_ssaComputeStack.push_back(c);
            data->m_decoupledData->m_targetIndex0 = c;
            c = context.m_currentSSARegisterCount++;
            context.m_ssaComputeStack.push_back(c);
            data->m_decoupledData->m_targetIndex1 = c;
        }
    }

    if (op == EndOpcode)
        ASSERT(context.m_ssaComputeStack.size() == 0);
#endif
}

void CodeBlock::fillExtraData()
{
    ExtraDataGenerateContext context(this);
    size_t idx = 0;
    size_t bytecodeCounter = 0;
    char* code = m_code.data();
    char* end = &m_code.data()[m_code.size()];
    while (&code[idx] < end) {
        ByteCode* currentCode = (ByteCode *)(&code[idx]);
        unsigned currentCount = bytecodeCounter;
        ByteCodeExtraData* ex = &m_extraData[currentCount];
        Opcode opcode = m_extraData[bytecodeCounter].m_opcode;
        bytecodeCounter++;

        fillExtraData(currentCode, ex, context, idx, bytecodeCounter);

        ASSERT(opcode == currentCode->m_orgOpcode);
        switch (opcode) {
#define NEXT_BYTE_CODE(code, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
        case code##Opcode:\
            idx += sizeof(code); \
            continue;
            FOR_EACH_BYTECODE_OP(NEXT_BYTE_CODE)
#undef  NEXT_BYTE_CODE
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        };
    }
}
#endif

#ifdef ENABLE_ESJIT
void CodeBlock::removeJITInfo()
{
#ifdef NDEBUG
    for (size_t i = 0; i < m_extraData.size(); i++) {
        if (m_extraData[i].m_decoupledData)
            delete m_extraData[i].m_decoupledData;
        m_extraData[i].m_decoupledData = nullptr;
    }
#endif
    m_byteCodeIndexesHaveToProfile.clear();
    m_byteCodeIndexesHaveToProfile.shrink_to_fit();
    RELEASE_ASSERT(!m_byteCodeIndexesHaveToProfile.capacity());
    m_byteCodePositionsHaveToProfile.clear();
    m_byteCodePositionsHaveToProfile.shrink_to_fit();
    RELEASE_ASSERT(!m_byteCodePositionsHaveToProfile.capacity());
}

void CodeBlock::removeJITCode()
{
    if (m_codeAlloc) {
        delete m_codeAlloc;
        m_codeAlloc = nullptr;
    }
    if (m_nanoJITDataAllocator) {
        delete m_nanoJITDataAllocator;
        m_nanoJITDataAllocator = nullptr;
    }
    m_cachedJITFunction = nullptr;
}

nanojit::CodeAlloc* CodeBlock::codeAlloc()
{
    if (!m_codeAlloc)
        m_codeAlloc = new nanojit::CodeAlloc(ESVMInstance::currentInstance()->getJITConfig());
    return m_codeAlloc;
}

nanojit::Allocator* CodeBlock::nanoJITDataAllocator()
{
    if (!m_nanoJITDataAllocator)
        m_nanoJITDataAllocator = new nanojit::Allocator();
    return m_nanoJITDataAllocator;
}
#endif

ByteCode::ByteCode(Opcode code)
{
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


CodeBlock* generateByteCode(CodeBlock* block, Node* node, ExecutableType type, ParserContextInformation& parserContextInformation, bool shouldGenerateByteCodeInstantly)
{
    // size_t dummy;
    // node->computeRoughCodeBlockSizeInWordSize(dummy);
    // CodeBlock* block = CodeBlock::create(node->roughCodeblockSizeInWordSize());
    if (!block) {
        block = CodeBlock::create(type);
    } else {
        ASSERT(!node);
        ASSERT(block->m_ast);
        node = block->m_ast;
    }

    ByteCodeGenerateContext context(block, parserContextInformation);
    context.m_shouldGenerateByteCodeInstantly = shouldGenerateByteCodeInstantly;
    // unsigned long start = ESVMInstance::tickCount();
    switch (type) {
    case ExecutableType::GlobalCode:
    case ExecutableType::EvalCode:
        node->generateStatementByteCode(block, context);
        break;
    case ExecutableType::FunctionCode:
        if (shouldGenerateByteCodeInstantly) {
            ((FunctionNode*)node)->initializeCodeBlock(block);
            context.m_hasArgumentsBinding = block->m_hasArgumentsBinding;
            ((FunctionNode*)node)->body()->generateStatementByteCode(block, context);
            block->pushCode(ReturnFunction(), context, node);
            block->m_ast = nullptr;
            block->m_hasCode = true;
        } else {
            block->m_ast = node;
            block->m_hasCode = false;
        }
        break;
    }
    // printf("codeBlock %d\n", (int)block->m_code.size());
    // unsigned long end = ESVMInstance::tickCount();
    // printf("generate code takes %lfms\n", (end-start)/1000.0);

    return block;
}

unsigned char popCountFromOpcode(ByteCode* code, Opcode opcode)
{
    if (opcode == CallFunctionOpcode) {
        CallFunction* c = (CallFunction*)code;
        return c->m_argmentCount + 1/* function */;
    } if (opcode == CallFunctionWithReceiverOpcode) {
        CallFunctionWithReceiver* c = (CallFunctionWithReceiver*)code;
        return c->m_argmentCount + 1/* receiver */ + 1/* function */;
    } else if (opcode == CallEvalFunctionOpcode) {
        CallEvalFunction* c = (CallEvalFunction*)code;
        return c->m_argmentCount;
    } else if (opcode == NewFunctionCallOpcode) {
        NewFunctionCall* c = (NewFunctionCall*)code;
        return c->m_argmentCount + 1/* function */;
    } else if (opcode == UnaryDeleteOpcode) {
        UnaryDelete* c = (UnaryDelete*)code;
        return c->m_isDeleteObjectKey? 2 : 0;
    }
#define FETCH_POP_COUNT_BYTE_CODE(code, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
    case code##Opcode: \
        ASSERT(popCount != -1); \
        return popCount;
    switch (opcode) {
        FOR_EACH_BYTECODE_OP(FETCH_POP_COUNT_BYTE_CODE);
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

unsigned char pushCountFromOpcode(ByteCode* code, Opcode opcode)
{
    if (opcode == CreateFunctionOpcode) {
        if (((CreateFunction *)code)->m_isDeclaration) {
            return 0;
        } else
            return 1;
    }
#define FETCH_PUSH_COUNT_BYTE_CODE(code, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
    case code##Opcode: \
        ASSERT(pushCount != -1); \
        return pushCount;
    switch (opcode) {
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
    switch (opcode) {
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
    printf("isStrict %d needs (Activation %d HeapAllocatedEC %d ComplexParameterCopy %d PrepareGenerateArgumentsObject %d HasArgumentsBinding %d)\n",
        codeBlock->m_isStrict, codeBlock->m_needsActivation, codeBlock->m_needsHeapAllocatedExecutionContext,
        codeBlock->m_needsComplexParameterCopy, codeBlock->m_needsToPrepareGenerateArgumentsObject, codeBlock->m_hasArgumentsBinding);
    size_t idx = 0;
    size_t bytecodeCounter = 0;
    char* code = codeBlock->m_code.data();
    char* end = &codeBlock->m_code.data()[codeBlock->m_code.size()];
    while (&code[idx] < end) {
        ByteCode* currentCode = (ByteCode *)(&code[idx]);
        unsigned currentCount = bytecodeCounter;
        ByteCodeExtraData* ex = &codeBlock->m_extraData[currentCount];
        if (currentCode->m_node)
            printf("%u\t\t%p\t(nodeinfo %d)\t\t", (unsigned)idx, currentCode, (int)currentCode->m_node->sourceLocation().m_lineNumber);
        else
            printf("%u\t\t%p\t(nodeinfo null)\t\t", (unsigned)idx, currentCode);

        printf("IdxInfo[%d,+%d,-%d]\t", ex->m_decoupledData->m_baseRegisterIndex, ex->m_decoupledData->m_registerIncrementCount, ex->m_decoupledData->m_registerDecrementCount);
#ifdef ENABLE_ESJIT
        printf("ssa->[");

        if (ex->m_decoupledData->m_targetIndex0 != -1) {
            printf("t: %d,", ex->m_decoupledData->m_targetIndex0);
        }

        if (ex->m_decoupledData->m_targetIndex1 != -1) {
            printf("t2: %d,", ex->m_decoupledData->m_targetIndex1);
        }

        for (size_t i = 0; i < ex->m_decoupledData->m_sourceIndexes.size() ; i ++) {
            printf("s: %d,", ex->m_decoupledData->m_sourceIndexes[i]);
        }
        printf("]");
#endif
        printf("\t");

        Opcode opcode = codeBlock->m_extraData[bytecodeCounter].m_opcode;
        bytecodeCounter++;
        ASSERT(opcode == currentCode->m_orgOpcode);

        switch (opcode) {
#define DUMP_BYTE_CODE(code, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
        case code##Opcode:\
            currentCode->dump(); \
            idx += sizeof(code); \
            continue;
            FOR_EACH_BYTECODE_OP(DUMP_BYTE_CODE)
#undef  DUMP_BYTE_CODE
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        };
    }
    for (size_t i = 0; i < codeBlock->m_innerIdentifiers.size(); i++) {
        auto ident = codeBlock->m_innerIdentifiers[i];
        printf("[Identifier %zu] name %20s heap(%d) immutable(%d) origin(%d)\n", i, ident.m_name.string()->utf8Data(),
            ident.m_flags.m_isHeapAllocated, ident.m_flags.m_bindingIsImmutable, (int)ident.m_flags.m_origin);
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
    while (&code[idx] < end) {
        Opcode opcode = block->m_extraData[bytecodeCounter].m_opcode;
        switch (opcode) {
#define DECLARE_EXECUTE_NEXTCODE(opcode, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
        case opcode##Opcode: \
            if (!JITSupported) { \
                auto result = names.insert(std::pair<std::string, size_t>(std::string(#opcode), 1)); \
                if (!result.second) \
                    names[std::string(#opcode)]++; \
            } \
            idx += sizeof(opcode); \
            bytecodeCounter++; \
            break;
            FOR_EACH_BYTECODE_OP(DECLARE_EXECUTE_NEXTCODE);
#undef DECLARE_EXECUTE_NEXTCODE
        case OpcodeKindEnd:
            break;
        }
    }
    for (auto it = names.begin(); it != names.end(); ++it) {
        printf("%s %zd", it->first.data(), it->second);
    }
    printf("\n");
}
#endif
}
