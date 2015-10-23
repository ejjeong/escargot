#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITFrontend.h"

#include "ESGraph.h"
#include "ESIR.h"

#include "bytecode/ByteCode.h"

namespace escargot {
namespace ESJIT {

#define DECLARE_BYTECODE_LENGTH(bytecode, pushCount, popCount, JITSupported) const int bytecode##Length = sizeof(bytecode);
    FOR_EACH_BYTECODE_OP(DECLARE_BYTECODE_LENGTH)
#undef DECLARE_BYTECODE_LENGTH

ESGraph* generateIRFromByteCode(CodeBlock* codeBlock)
{
    ESGraph* graph = ESGraph::create(codeBlock);

//#ifndef NDEBUG
//    if (ESVMInstance::currentInstance()->m_verboseJIT)
//        dumpBytecode(codeBlock);
//#endif

    size_t idx = 0;
    size_t bytecodeCounter = 0;
    size_t callInfoIndex = 0;
    char* code = codeBlock->m_code.data();

    std::map<int, ESBasicBlock*> basicBlockMapping;

    ESBasicBlock *entryBlock = ESBasicBlock::create(graph);
    basicBlockMapping[idx] = entryBlock;
    ESBasicBlock* currentBlock = entryBlock;
    ByteCode* currentCode;

    char* end = &codeBlock->m_code.data()[codeBlock->m_code.size()];
    while(&code[idx] < end) {
        currentCode = (ByteCode *)(&code[idx]);
        Opcode opcode = codeBlock->m_extraData[bytecodeCounter].m_opcode;

        // Update BasicBlock information
        // TODO: find a better way to this (e.g. using AST, write information to bytecode..)
        if (ESBasicBlock* generatedBlock = basicBlockMapping[idx]) {
            if (currentBlock != generatedBlock && !currentBlock->endsWithJumpOrBranch()) {
                currentBlock->addChild(generatedBlock);
                generatedBlock->addParent(currentBlock);
              }
            currentBlock = generatedBlock;
            if (currentBlock->index() == SIZE_MAX) {
                for (int i = graph->basicBlockSize() - 1; i >= 0; i--) {
                    int blockIndex = graph->basicBlock(i)->index();
                    if (blockIndex >= 0) {
                        currentBlock->setIndexLater(blockIndex + 1);
                        graph->push(currentBlock);
                        break;
                       }
                  }
              }
        }
        // printf("parse idx %lu with BasicBlock %lu\n", idx, currentBlock->index());

#define INIT_BYTECODE(ByteCode) \
            ByteCode* bytecode = (ByteCode*)currentCode; \
            SSAIndex* ssaIndex = codeBlock->getSSAIndex(bytecodeCounter); \
            ASSERT(codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount < 2); \
            if (ssaIndex->m_targetIndex >= 0 && codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount == 1) \
            graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
#define NEXT_BYTECODE(ByteCode) \
            idx += sizeof(ByteCode); \
            bytecodeCounter++;
        switch(opcode) {
        case PushOpcode:
        {
            INIT_BYTECODE(Push);
//            graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* literal;
            if (bytecode->m_value.isInt32()) {
                literal = ConstantIntIR::create(ssaIndex->m_targetIndex, bytecode->m_value.asInt32());
            } else if (bytecode->m_value.isDouble()) {
                literal = ConstantDoubleIR::create(ssaIndex->m_targetIndex, bytecode->m_value.asDouble());
            } else if (bytecode->m_value.isBoolean()) {
                literal = ConstantBooleanIR::create(ssaIndex->m_targetIndex, bytecode->m_value.asBoolean());
            } else if (bytecode->m_value.isESPointer()) {
                ESPointer* p = bytecode->m_value.asESPointer();
                if (p->isESString())
                    literal = ConstantStringIR::create(ssaIndex->m_targetIndex, bytecode->m_value.asESString());
                else
                    goto unsupported;
            } else if (bytecode->m_value.isNull() || bytecode->m_value.isUndefined()) {
                literal = ConstantESValueIR::create(ssaIndex->m_targetIndex, bytecode->m_value);
            } else
                goto unsupported;
            currentBlock->push(literal);
            NEXT_BYTECODE(Push);
            break;
        }
        case PopExpressionStatementOpcode: {
            graph->increaseFollowingPopCountOf(graph->lastStackPosSettingTargetIndex());
            NEXT_BYTECODE(PopExpressionStatement);
            break;
        }
        case PopOpcode: {
            graph->increaseFollowingPopCountOf(graph->lastStackPosSettingTargetIndex());
            NEXT_BYTECODE(Pop);
            break;
        }
        case PushIntoTempStackOpcode:
            NEXT_BYTECODE(PushIntoTempStack);
            break;
        case PopFromTempStackOpcode:
            NEXT_BYTECODE(PopFromTempStack);
            break;
        case GetByIdOpcode:
        {
            INIT_BYTECODE(GetById);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* getVarGeneric = GetVarGenericIR::create(ssaIndex->m_targetIndex, bytecode, bytecode->m_name, bytecode->m_name.string()); // FIXME store only bytecode, get name from that
            currentBlock->push(getVarGeneric);
            bytecode->m_profile.updateProfiledType();
            graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            NEXT_BYTECODE(GetById);
            break;
        }
        case GetByGlobalIndexOpcode:
        {
            INIT_BYTECODE(GetByGlobalIndex);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* getGlobalVarGeneric = GetGlobalVarGenericIR::create(ssaIndex->m_targetIndex, bytecode, bytecode->m_name); // FIXME store only bytecode, get name from that
            currentBlock->push(getGlobalVarGeneric);
            bytecode->m_profile.updateProfiledType();
            graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            NEXT_BYTECODE(GetByGlobalIndex);
            break;
        }
        case GetByIndexOpcode:
        {
            INIT_BYTECODE(GetByIndex);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            // TODO: load from local variable should not be a heap load.
            if (bytecode->m_index < codeBlock->m_params.size()) {
                ESIR* getArgument = GetArgumentIR::create(ssaIndex->m_targetIndex, bytecode->m_index);
                currentBlock->push(getArgument);
            } else {
                ESIR* getVar = GetVarIR::create(ssaIndex->m_targetIndex, bytecode->m_index, 0);
                currentBlock->push(getVar);
            }
            bytecode->m_profile.updateProfiledType();
            graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            NEXT_BYTECODE(GetByIndex);
            break;
        }
        case GetByIndexWithActivationOpcode:
        {
            INIT_BYTECODE(GetByIndexWithActivation);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* getVar = GetVarIR::create(ssaIndex->m_targetIndex, bytecode->m_index, bytecode->m_upIndex);
            currentBlock->push(getVar);
            bytecode->m_profile.updateProfiledType();
            graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            NEXT_BYTECODE(GetByIndexWithActivation);
            break;
        }
        case SetByIdOpcode:
        {
            INIT_BYTECODE(SetById);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* setVarGeneric = SetVarGenericIR::create(ssaIndex->m_targetIndex, bytecode, ssaIndex->m_srcIndex1, &bytecode->m_name, bytecode->m_name.string());
            currentBlock->push(setVarGeneric);
            NEXT_BYTECODE(SetById);
            break;
        }
        case SetByIndexOpcode:
        {
            INIT_BYTECODE(SetByIndex);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* setVar = SetVarIR::create(ssaIndex->m_targetIndex, bytecode->m_index, 0, ssaIndex->m_srcIndex1);
            currentBlock->push(setVar);
            NEXT_BYTECODE(SetByIndex);
            break;
        }
        case SetByIndexWithActivationOpcode:
        {
            INIT_BYTECODE(SetByIndexWithActivation);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* setVar = SetVarIR::create(ssaIndex->m_targetIndex, bytecode->m_index, bytecode->m_upIndex, ssaIndex->m_srcIndex1);
            currentBlock->push(setVar);
            NEXT_BYTECODE(SetByIndexWithActivation);
            break;
        }
        case SetByGlobalIndexOpcode:
        {
            INIT_BYTECODE(SetByGlobalIndex);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* setGlobalVarGeneric = SetGlobalVarGenericIR::create(ssaIndex->m_targetIndex, bytecode, ssaIndex->m_srcIndex1, bytecode->m_name);
            currentBlock->push(setGlobalVarGeneric);
            NEXT_BYTECODE(SetByGlobalIndex);
            break;
        }
        case SetObjectOpcode:
        {
            INIT_BYTECODE(SetObject);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* setObject = SetObjectIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2, ssaIndex->m_targetIndex - 1);
            currentBlock->push(setObject);
            NEXT_BYTECODE(SetObject);
            break;
        }
        case SetObjectPreComputedCaseOpcode:
        {
            INIT_BYTECODE(SetObjectPreComputedCase);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* setObject = SetObjectPreComputedIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_targetIndex - 1, bytecode);
            currentBlock->push(setObject);
            NEXT_BYTECODE(SetObjectPreComputedCase);
            break;
        }
        case CreateBindingOpcode:
            goto unsupported;
            NEXT_BYTECODE(CreateBinding);
            break;
        case EqualOpcode:
        {
            INIT_BYTECODE(Equal);
            ESIR* equalIR = EqualIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(equalIR);
            NEXT_BYTECODE(Equal);
            break;
        }
        case NotEqualOpcode:
            goto unsupported;
            NEXT_BYTECODE(NotEqual);
            break;
        case StrictEqualOpcode:
            goto unsupported;
            NEXT_BYTECODE(StrictEqual);
            break;
        case NotStrictEqualOpcode:
            goto unsupported;
            NEXT_BYTECODE(NotStrictEqual);
            break;
        case BitwiseAndOpcode:
        {
            INIT_BYTECODE(BitwiseAnd);
            ESIR* bitwiseAndIR = BitwiseAndIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(bitwiseAndIR);
            NEXT_BYTECODE(BitwiseAnd);
            break;
        }
        case BitwiseOrOpcode:
        {
            INIT_BYTECODE(BitwiseOr);
            ESIR* bitwiseOrIR = BitwiseOrIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(bitwiseOrIR);
            NEXT_BYTECODE(BitwiseOr);
            break;
        }
        case BitwiseXorOpcode:
        {
            INIT_BYTECODE(BitwiseXor);
            ESIR* bitwiseXorIR = BitwiseXorIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(bitwiseXorIR);
            NEXT_BYTECODE(BitwiseXor);
            break;
        }
        case LeftShiftOpcode:
        {
            INIT_BYTECODE(LeftShift);
            ESIR* leftShiftIR = LeftShiftIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(leftShiftIR);
            NEXT_BYTECODE(LeftShift);
            break;
        }
        case SignedRightShiftOpcode:
        {
            INIT_BYTECODE(SignedRightShift);
            ESIR* signedRightShiftIR = SignedRightShiftIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(signedRightShiftIR);
            NEXT_BYTECODE(SignedRightShift);
            break;
        }
        case UnsignedRightShiftOpcode:
        {
            INIT_BYTECODE(UnsignedRightShift);
            ESIR* unsignedRightShiftIR = UnsignedRightShiftIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(unsignedRightShiftIR);
            NEXT_BYTECODE(UnsignedRightShift);
            break;
        }
        case LessThanOpcode:
        {
            INIT_BYTECODE(LessThan);
            ESIR* lessThanIR = LessThanIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(lessThanIR);
            NEXT_BYTECODE(LessThan);
            break;
        }
        case LessThanOrEqualOpcode:
        {
            INIT_BYTECODE(LessThanOrEqual);
            ESIR* lessThanOrEqualIR = LessThanOrEqualIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(lessThanOrEqualIR);
            NEXT_BYTECODE(LessThanOrEqual);
            break;
        }
        case GreaterThanOpcode:
        {
            INIT_BYTECODE(GreaterThan);
            ESIR* lessThanIR = GreaterThanIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(lessThanIR);
            NEXT_BYTECODE(GreaterThan);
            break;
        }
        case GreaterThanOrEqualOpcode:
        {
            INIT_BYTECODE(GreaterThanOrEqual);
            ESIR* lessThanOrEqualIR = GreaterThanOrEqualIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(lessThanOrEqualIR);
            NEXT_BYTECODE(GreaterThanOrEqual);
            break;
        }
        case PlusOpcode:
        {
            // TODO
            // 1. if both arguments have number type then append NumberPlus
            // 2. else if either one of arguments has string type then append StringPlus
            // 3. else append general Plus
            INIT_BYTECODE(Plus);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* genericPlusIR = GenericPlusIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(genericPlusIR);
            NEXT_BYTECODE(Plus);
            break;
        }
        case MinusOpcode:
        {
            INIT_BYTECODE(Minus);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* minusIR = MinusIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(minusIR);
            NEXT_BYTECODE(Minus);
            break;
        }
        case MultiplyOpcode:
        {
            INIT_BYTECODE(Multiply);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* genericMultiplyIR = GenericMultiplyIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(genericMultiplyIR);
            NEXT_BYTECODE(Multiply);
            break;
        }
        case DivisionOpcode:
        {
            INIT_BYTECODE(Division);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* genericDivisionIR = GenericDivisionIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(genericDivisionIR);
            NEXT_BYTECODE(Division);
            break;
        }
        case ModOpcode:
        {
            INIT_BYTECODE(Mod);
            ESIR* genericModIR = GenericModIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(genericModIR);
            NEXT_BYTECODE(Mod);
            break;
        }
        case StringInOpcode:
            goto unsupported;
            NEXT_BYTECODE(StringIn);
            break;
        case BitwiseNotOpcode:
        {
            INIT_BYTECODE(BitwiseNot);
            ESIR* bitwiseNotIR = BitwiseNotIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1);
            currentBlock->push(bitwiseNotIR);
            NEXT_BYTECODE(BitwiseNot);
            break;
        }
        case LogicalNotOpcode:
            goto unsupported;
            NEXT_BYTECODE(LogicalNot);
            break;
        case UnaryMinusOpcode:
        {
            INIT_BYTECODE(UnaryMinus);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* UnaryMinusIR = UnaryMinusIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1);
            currentBlock->push(UnaryMinusIR);
            NEXT_BYTECODE(UnaryMinus);
            break;
        }
        case UnaryPlusOpcode:
            goto unsupported;
            NEXT_BYTECODE(UnaryPlus);
            break;
        case UnaryTypeOfOpcode:
            goto unsupported;
            NEXT_BYTECODE(UnaryTypeOf);
            break;
        case UnaryDeleteOpcode:
            goto unsupported;
            NEXT_BYTECODE(UnaryDelete);
            break;
        case ToNumberOpcode:
        {
            INIT_BYTECODE(ToNumber);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* toNumberIR = ToNumberIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1);
            currentBlock->push(toNumberIR);
            NEXT_BYTECODE(ToNumber);
            break;
        }
        case IncrementOpcode:
        {
            INIT_BYTECODE(Increment);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            ESIR* incrementIR = IncrementIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1);
            currentBlock->push(incrementIR);
            NEXT_BYTECODE(Increment);
            break;
        }
        case DecrementOpcode:
            goto unsupported;
            NEXT_BYTECODE(Decrement);
            break;
        case CreateObjectOpcode:
            goto unsupported;
            NEXT_BYTECODE(CreateObject);
            break;
        case CreateArrayOpcode:
            goto unsupported;
            NEXT_BYTECODE(CreateArray);
            break;
        case GetObjectOpcode:
        case GetObjectAndPushObjectOpcode:
        {
            GetObject* bytecode = (GetObject*)currentCode;
            SSAIndex* ssaIndex = codeBlock->getSSAIndex(bytecodeCounter);
            ASSERT(codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount < 3);
            if (codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount == 1)
                graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            else if (codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount == 2) {
                graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter].m_baseRegisterIndex);
                graph->setOperandStackPos(ssaIndex->m_targetIndex - 1, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
             }
            else
                graph->setOperandStackPos(ssaIndex->m_targetIndex, -1);
            bytecode->m_profile.updateProfiledType();
            graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            bytecode->m_profile.updateProfiledType();
            graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            GetObjectIR* getObjectIR = GetObjectIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2, bytecode);
            currentBlock->push(getObjectIR);
            NEXT_BYTECODE(GetObject);
            break;
        }
        case GetObjectWithPeekingOpcode:
            goto unsupported;
            NEXT_BYTECODE(GetObjectWithPeeking);
            break;
        case GetObjectPreComputedCaseOpcode:
        case GetObjectPreComputedCaseAndPushObjectOpcode:
        {
            GetObjectPreComputedCase* bytecode = (GetObjectPreComputedCase*)currentCode;
            SSAIndex* ssaIndex = codeBlock->getSSAIndex(bytecodeCounter);
            ASSERT(codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount < 3);
            if (codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount == 1)
                graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            else if (codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount == 2) {
                graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter].m_baseRegisterIndex);
                graph->setOperandStackPos(ssaIndex->m_targetIndex - 1, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
             }
            else
                graph->setOperandStackPos(ssaIndex->m_targetIndex, -1);
            bytecode->m_profile.updateProfiledType();
            graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            GetObjectPreComputedIR* getObjectPreComputedIR = GetObjectPreComputedIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1,
                    bytecode->m_cachedIndex, bytecode);
            currentBlock->push(getObjectPreComputedIR);
            NEXT_BYTECODE(GetObjectPreComputedCase);
            break;
        }
        case GetObjectWithPeekingPreComputedCaseOpcode:
            goto unsupported;
            NEXT_BYTECODE(GetObjectWithPeekingPreComputedCase);
            break;
        case CreateFunctionOpcode:
            goto unsupported;
            NEXT_BYTECODE(ExecuteNativeFunction);
            break;
        case CallFunctionOpcode:
        {
            INIT_BYTECODE(CallFunction);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            int calleeIndex = codeBlock->m_functionCallInfos[callInfoIndex++];
            int receiverIndex = codeBlock->m_functionCallInfos[callInfoIndex++];
            int argumentCount = codeBlock->m_functionCallInfos[callInfoIndex++];
            int* argumentIndexes = (int*) alloca (sizeof(int) * argumentCount);
            for (int i=0; i<argumentCount; i++)
                argumentIndexes[i] = codeBlock->m_functionCallInfos[callInfoIndex++];
            CallJSIR* callJSIR = CallJSIR::create(ssaIndex->m_targetIndex, calleeIndex, receiverIndex, argumentCount, argumentIndexes);
            currentBlock->push(callJSIR);
            bytecode->m_profile.updateProfiledType();
            graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            NEXT_BYTECODE(CallFunction);
            break;
        }

        case CallFunctionWithReceiverOpcode:
        {
            INIT_BYTECODE(CallFunctionWithReceiver);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            int calleeIndex = codeBlock->m_functionCallInfos[callInfoIndex++];
            callInfoIndex++;
            int receiverIndex = calleeIndex - 1;
            int argumentCount = codeBlock->m_functionCallInfos[callInfoIndex++];
            int* argumentIndexes = (int*) alloca (sizeof(int) * argumentCount);
            for (int i=0; i<argumentCount; i++)
                argumentIndexes[i] = codeBlock->m_functionCallInfos[callInfoIndex++];
            CallJSIR* callJSIR = CallJSIR::create(ssaIndex->m_targetIndex, calleeIndex, receiverIndex, argumentCount, argumentIndexes);
            currentBlock->push(callJSIR);
            bytecode->m_profile.updateProfiledType();
            graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            NEXT_BYTECODE(CallFunctionWithReceiver);
            break;
        }

        case NewFunctionCallOpcode:
        {
            INIT_BYTECODE(NewFunctionCall);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            int calleeIndex = codeBlock->m_functionCallInfos[callInfoIndex++];
            int receiverIndex = codeBlock->m_functionCallInfos[callInfoIndex++];
            int argumentCount = codeBlock->m_functionCallInfos[callInfoIndex++];
            int* argumentIndexes = (int*) alloca (sizeof(int) * argumentCount);
            for (int i=0; i<argumentCount; i++)
                argumentIndexes[i] = codeBlock->m_functionCallInfos[callInfoIndex++];
            CallNewJSIR* callNewJSIR = CallNewJSIR::create(ssaIndex->m_targetIndex, calleeIndex, receiverIndex, argumentCount, argumentIndexes);
            currentBlock->push(callNewJSIR);
            bytecode->m_profile.updateProfiledType();
            graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            NEXT_BYTECODE(NewFunctionCall);
            break;
        }
        case ReturnFunctionOpcode:
        {
            INIT_BYTECODE(ReturnFunction);
            ReturnIR* returnIR = ReturnIR::create(-1);
            currentBlock->push(returnIR);
            NEXT_BYTECODE(ReturnFunction);
            break;
        }
        case ReturnFunctionWithValueOpcode:
        {
            INIT_BYTECODE(ReturnFunctionWithValue);
            ReturnWithValueIR* returnWithValueIR = ReturnWithValueIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1);
            currentBlock->push(returnWithValueIR);
            NEXT_BYTECODE(ReturnFunctionWithValue);
            break;
        }
        case JumpOpcode:
        {
            INIT_BYTECODE(Jump);
            ESBasicBlock* targetBlock;
            if (basicBlockMapping.find(bytecode->m_jumpPosition) != basicBlockMapping.end()) {
                targetBlock = basicBlockMapping[bytecode->m_jumpPosition];
                targetBlock->addParent(currentBlock);
                currentBlock->addChild(targetBlock);
            } else
                targetBlock = ESBasicBlock::create(graph, currentBlock, true);
            JumpIR* jumpIR = JumpIR::create(ssaIndex->m_targetIndex, targetBlock);
            currentBlock->push(jumpIR);
            basicBlockMapping[bytecode->m_jumpPosition] = targetBlock;
            NEXT_BYTECODE(Jump);
            break;
        }
        case JumpIfTopOfStackValueIsFalseOpcode:
        {
            INIT_BYTECODE(JumpIfTopOfStackValueIsFalse);

            ESBasicBlock* trueBlock = ESBasicBlock::create(graph, currentBlock);
            ESBasicBlock* falseBlock = ESBasicBlock::create(graph, currentBlock, true);

            BranchIR* branchIR = BranchIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, trueBlock, falseBlock);
            currentBlock->push(branchIR);

            basicBlockMapping[idx + sizeof(JumpIfTopOfStackValueIsFalse)] = trueBlock;
            basicBlockMapping[bytecode->m_jumpPosition] = falseBlock;

            NEXT_BYTECODE(JumpIfTopOfStackValueIsFalse);
            break;
        }
        case JumpIfTopOfStackValueIsTrueOpcode:
            goto unsupported;
            NEXT_BYTECODE(JumpIfTopOfStackValueIsTrue);
            break;
        case JumpIfTopOfStackValueIsFalseWithPeekingOpcode:
            goto unsupported;
            NEXT_BYTECODE(JumpIfTopOfStackValueIsFalseWithPeeking);
            break;
        case JumpIfTopOfStackValueIsTrueWithPeekingOpcode:
            goto unsupported;
            NEXT_BYTECODE(JumpIfTopOfStackValueIsTrueWithPeeking);
            break;
        case JumpAndPopIfTopOfStackValueIsTrueOpcode:
        {
            INIT_BYTECODE(JumpAndPopIfTopOfStackValueIsTrue);
            ESBasicBlock* falseBlock = ESBasicBlock::create(graph, currentBlock);
            ESBasicBlock* trueBlock = ESBasicBlock::create(graph, currentBlock, true);

            BranchIR* branchIR = BranchIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, trueBlock, falseBlock);
            currentBlock->push(branchIR);

            basicBlockMapping[idx + sizeof(JumpAndPopIfTopOfStackValueIsTrue)] = falseBlock;
            basicBlockMapping[bytecode->m_jumpPosition] = trueBlock;
            NEXT_BYTECODE(JumpAndPopIfTopOfStackValueIsTrue);
            break;
        }
        case DuplicateTopOfStackValueOpcode:
        {
            INIT_BYTECODE(DuplicateTopOfStackValue);
            MoveIR* moveIR = MoveIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1);
            currentBlock->push(moveIR);
            NEXT_BYTECODE(DuplicateTopOfStackValue);
            break;
        }
        case LoopStartOpcode:
        {
            INIT_BYTECODE(LoopStart);
            ESBasicBlock* loopBlock = ESBasicBlock::create(graph);
            basicBlockMapping[idx + sizeof(LoopStart)] = loopBlock;
            NEXT_BYTECODE(LoopStart);
            break;
        }
        case ThrowOpcode:
            goto unsupported;
            NEXT_BYTECODE(Throw);
            break;
        case AllocPhiOpcode:
        {
            INIT_BYTECODE(AllocPhi);
            AllocPhiIR* allocPhiIR = AllocPhiIR::create(ssaIndex->m_targetIndex);
            currentBlock->push(allocPhiIR);
            NEXT_BYTECODE(AllocPhi);
            break;
        }
        case StorePhiOpcode:
        {
           INIT_BYTECODE(StorePhi);
           StorePhiIR* storePhiIR = StorePhiIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
           currentBlock->push(storePhiIR);
           NEXT_BYTECODE(StorePhi);
           break;
        }
        case LoadPhiOpcode:
        {
           INIT_BYTECODE(LoadPhi);
           LoadPhiIR* loadPhiIR = LoadPhiIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
           currentBlock->push(loadPhiIR);
           NEXT_BYTECODE(LoadPhi);
           break;
        }
        case ThisOpcode:
        {
            INIT_BYTECODE(This);
            //graph->setOperandStackPos(ssaIndex->m_targetIndex, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
            bytecode->m_profile.updateProfiledType();
            graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            GetThisIR* getThisIR = GetThisIR::create(ssaIndex->m_targetIndex);
            currentBlock->push(getThisIR);
            NEXT_BYTECODE(This);
            break;
        }
        case EnumerateObjectOpcode:
        case EnumerateObjectKeyOpcode:
            goto unsupported;
        case PrintSpAndBpOpcode:
            goto unsupported;
        case EndOpcode:
            goto postprocess;
        default:
#ifndef NDEBUG
            printf("Invalid Opcode %s\n", getByteCodeName(opcode));
#endif
            RELEASE_ASSERT_NOT_REACHED();
        }
#undef INIT_BYTECODE
#undef NEXT_BYTECODE
    }

postprocess:
#ifndef NDEBUG
    if (ESVMInstance::currentInstance()->m_verboseJIT)
        graph->dump(std::cout);
#endif
    return graph;

unsupported:
    LOG_VJ("Unsupported case in ByteCode %s (idx %zu) (while parsing in FrontEnd)\n", getByteCodeName(codeBlock->m_extraData[bytecodeCounter].m_opcode), idx);
    return nullptr;
}

}}
#endif
