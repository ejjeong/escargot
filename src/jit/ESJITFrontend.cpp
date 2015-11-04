#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITFrontend.h"

#include "ESGraph.h"
#include "ESIR.h"

#include "bytecode/ByteCode.h"

namespace escargot {
namespace ESJIT {

#define DECLARE_BYTECODE_LENGTH(bytecode, pushCount, popCount, peekCount, JITSupported, hasProfileData) const int bytecode##Length = sizeof(bytecode);
FOR_EACH_BYTECODE_OP(DECLARE_BYTECODE_LENGTH)
#undef DECLARE_BYTECODE_LENGTH

ESGraph* generateIRFromByteCode(CodeBlock* codeBlock)
{
    ESGraph* graph = ESGraph::create(codeBlock);

    // #ifndef NDEBUG
    // if (ESVMInstance::currentInstance()->m_verboseJIT)
    // dumpBytecode(codeBlock);
    // #endif

    size_t idx = 0;
    size_t bytecodeCounter = 0;
    size_t callInfoIndex = 0;
    char* code = codeBlock->m_code.data();

    std::map<int, ESBasicBlock*> basicBlockMapping;
    // TODO
    // std::unordered_map<int, ESBasicBlock*, std::hash<int>, std::equal_to<int>, gc_allocator<std::pair<const int, ESBasicBlock *> > > basicBlockMapping;
    std::map<int, ESIR*> backWordJumpMapping;

    ESBasicBlock* entryBlock = ESBasicBlock::create(graph);
    basicBlockMapping[idx] = entryBlock;
    ESBasicBlock* currentBlock = entryBlock;
    ByteCode* currentCode;

    char* end = &codeBlock->m_code.data()[codeBlock->m_code.size()];
    while (&code[idx] < end) {
        currentCode = (ByteCode *)(&code[idx]);
        Opcode opcode = codeBlock->m_extraData[bytecodeCounter].m_opcode;

        // Update BasicBlock information
        // TODO: find a better way to this (e.g. using AST, write information to bytecode..)
        if (basicBlockMapping.find(idx) != basicBlockMapping.end()) {
            ESBasicBlock* generatedBlock = basicBlockMapping.find(idx)->second;
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
            ByteCodeExtraData* extraData = &codeBlock->m_extraData[bytecodeCounter]; \
            ASSERT(codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount < 2); \
            if (codeBlock->m_extraData[bytecodeCounter].m_targetIndex0 >= 0 && codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount == 1) \
                graph->setOperandStackPos(codeBlock->m_extraData[bytecodeCounter].m_targetIndex0, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
#define NEXT_BYTECODE(ByteCode) \
            idx += sizeof(ByteCode); \
            bytecodeCounter++;
        switch (opcode) {
        case PushOpcode:
            {
                INIT_BYTECODE(Push);
                ESIR* literal;
                if (bytecode->m_value.isInt32()) {
                    literal = ConstantIntIR::create(extraData->m_targetIndex0, bytecode->m_value.asInt32());
                    graph->setOperandType(extraData->m_targetIndex0, TypeInt32);
                } else if (bytecode->m_value.isDouble()) {
                    literal = ConstantDoubleIR::create(extraData->m_targetIndex0, bytecode->m_value.asDouble());
                    graph->setOperandType(extraData->m_targetIndex0, TypeDouble);
                } else if (bytecode->m_value.isBoolean()) {
                    literal = ConstantBooleanIR::create(extraData->m_targetIndex0, bytecode->m_value.asBoolean());
                    graph->setOperandType(extraData->m_targetIndex0, TypeBoolean);
                } else if (bytecode->m_value.isNull() || bytecode->m_value.isUndefined() || bytecode->m_value.isEmpty()) {
                    literal = ConstantESValueIR::create(extraData->m_targetIndex0, ESValue::toRawDouble(bytecode->m_value));
                    graph->setOperandType(extraData->m_targetIndex0, Type::getType(bytecode->m_value));
                } else if (bytecode->m_value.isESPointer()) {
                    ESPointer* p = bytecode->m_value.asESPointer();
                    if (p->isESString()) {
                        literal = ConstantStringIR::create(extraData->m_targetIndex0, bytecode->m_value.asESString());
                        graph->setOperandType(extraData->m_targetIndex0, TypeString);
                    } else {
                        literal = ConstantPointerIR::create(extraData->m_targetIndex0, bytecode->m_value.asESPointer());
                        graph->setOperandType(extraData->m_targetIndex0, TypePointer);
                    }
                } else
                    goto unsupported;
                currentBlock->push(literal);
                NEXT_BYTECODE(Push);
                break;
        }
        case PopExpressionStatementOpcode:
            {
                graph->increaseFollowingPopCountOf(graph->lastStackPosSettingTargetIndex());
                NEXT_BYTECODE(PopExpressionStatement);
                break;
            }
        case PopOpcode:
            {
                graph->increaseFollowingPopCountOf(graph->lastStackPosSettingTargetIndex());
                NEXT_BYTECODE(Pop);
                break;
            }
        case PushIntoTempStackOpcode:
            {
                INIT_BYTECODE(PushIntoTempStack);
                MoveIR* moveIR = MoveIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(moveIR);
                NEXT_BYTECODE(PushIntoTempStack);
                break;
            }
        case PopFromTempStackOpcode:
            {
                INIT_BYTECODE(PopFromTempStack);
                MoveIR* moveIR = MoveIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(moveIR);
                NEXT_BYTECODE(PopFromTempStack);
                break;
            }
        case GetByIdOpcode:
            {
                INIT_BYTECODE(GetById);
                ESIR* getVarGeneric = GetVarGenericIR::create(extraData->m_targetIndex0, bytecode);
                currentBlock->push(getVarGeneric);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                NEXT_BYTECODE(GetById);
                break;
            }
        case GetByIdWithoutExceptionOpcode:
            {
                INIT_BYTECODE(GetByIdWithoutException);
                ESIR* getVarWithoutException = GetVarGenericWithoutExceptionIR::create(extraData->m_targetIndex0, bytecode);
                currentBlock->push(getVarWithoutException);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                NEXT_BYTECODE(GetByIdWithoutException);
                break;
            }
        case GetByGlobalIndexOpcode:
            {
                INIT_BYTECODE(GetByGlobalIndex);
                ESIR* getGlobalVarGeneric = GetGlobalVarGenericIR::create(extraData->m_targetIndex0, bytecode, bytecode->m_name); // FIXME store only bytecode, get name from that
                currentBlock->push(getGlobalVarGeneric);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                NEXT_BYTECODE(GetByGlobalIndex);
                break;
            }
        case GetByIndexOpcode:
            {
                INIT_BYTECODE(GetByIndex);
                // TODO: load from local variable should not be a heap load.
                if (bytecode->m_index < codeBlock->m_params.size()) {
                    ESIR* getArgument = GetArgumentIR::create(extraData->m_targetIndex0, bytecode->m_index);
                    currentBlock->push(getArgument);
                } else {
                    ESIR* getVar = GetVarIR::create(extraData->m_targetIndex0, bytecode->m_index, 0, false);
                    currentBlock->push(getVar);
                }
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                NEXT_BYTECODE(GetByIndex);
                break;
            }
        case GetByIndexWithActivationOpcode:
            {
                INIT_BYTECODE(GetByIndexWithActivation);
                ESIR* getVar = GetVarIR::create(extraData->m_targetIndex0, bytecode->m_index, bytecode->m_upIndex, true);
                currentBlock->push(getVar);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                NEXT_BYTECODE(GetByIndexWithActivation);
                break;
            }
        case SetByIdOpcode:
            {
                INIT_BYTECODE(SetById);
                ESIR* setVarGeneric = SetVarGenericIR::create(extraData->m_targetIndex0, bytecode, extraData->m_sourceIndexes[0], &bytecode->m_name, bytecode->m_name.string());
                currentBlock->push(setVarGeneric);
                NEXT_BYTECODE(SetById);
                break;
            }
        case SetByIndexOpcode:
            {
                INIT_BYTECODE(SetByIndex);
                ESIR* setVar = SetVarIR::create(extraData->m_targetIndex0, bytecode->m_index, 0, extraData->m_sourceIndexes[0], false);
                currentBlock->push(setVar);
                NEXT_BYTECODE(SetByIndex);
                break;
            }
        case SetByIndexWithActivationOpcode:
            {
                INIT_BYTECODE(SetByIndexWithActivation);
                ESIR* setVar = SetVarIR::create(extraData->m_targetIndex0, bytecode->m_index, bytecode->m_upIndex, extraData->m_sourceIndexes[0], true);
                currentBlock->push(setVar);
                NEXT_BYTECODE(SetByIndexWithActivation);
                break;
            }
        case SetByGlobalIndexOpcode:
            {
                INIT_BYTECODE(SetByGlobalIndex);
                ESIR* setGlobalVarGeneric = SetGlobalVarGenericIR::create(extraData->m_targetIndex0, bytecode, extraData->m_sourceIndexes[0], bytecode->m_name);
                currentBlock->push(setGlobalVarGeneric);
                NEXT_BYTECODE(SetByGlobalIndex);
                break;
            }
        case SetObjectOpcode:
            {
                INIT_BYTECODE(SetObject);
                ESIR* setObject = SetObjectIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1], extraData->m_sourceIndexes[2]);
                currentBlock->push(setObject);
                NEXT_BYTECODE(SetObject);
                break;
            }
        case SetObjectPreComputedCaseOpcode:
            {
                INIT_BYTECODE(SetObjectPreComputedCase);
                ESIR* setObject = SetObjectPreComputedIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1], bytecode);
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
                ESIR* equalIR = EqualIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(equalIR);
                NEXT_BYTECODE(Equal);
                break;
            }
        case NotEqualOpcode:
            {
                INIT_BYTECODE(NotEqual);
                ESIR* notEqualIR = NotEqualIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(notEqualIR);
                NEXT_BYTECODE(NotEqual);
                break;
            }
        case StrictEqualOpcode:
            {
                INIT_BYTECODE(StrictEqual);
                ESIR* strictEqualIR = StrictEqualIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(strictEqualIR);
                NEXT_BYTECODE(StrictEqual);
                break;
            }
        case NotStrictEqualOpcode:
            {
                INIT_BYTECODE(NotStrictEqual);
                ESIR* notStrictEqualIR = NotStrictEqualIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(notStrictEqualIR);
                NEXT_BYTECODE(NotStrictEqual);
                break;
            }
        case BitwiseAndOpcode:
            {
                INIT_BYTECODE(BitwiseAnd);
                ESIR* bitwiseAndIR = BitwiseAndIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(bitwiseAndIR);
                NEXT_BYTECODE(BitwiseAnd);
                break;
            }
        case BitwiseOrOpcode:
            {
                INIT_BYTECODE(BitwiseOr);
                ESIR* bitwiseOrIR = BitwiseOrIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(bitwiseOrIR);
                NEXT_BYTECODE(BitwiseOr);
                break;
            }
        case BitwiseXorOpcode:
            {
                INIT_BYTECODE(BitwiseXor);
                ESIR* bitwiseXorIR = BitwiseXorIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(bitwiseXorIR);
                NEXT_BYTECODE(BitwiseXor);
                break;
            }
        case LeftShiftOpcode:
            {
                INIT_BYTECODE(LeftShift);
                ESIR* leftShiftIR = LeftShiftIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(leftShiftIR);
                NEXT_BYTECODE(LeftShift);
                break;
            }
        case SignedRightShiftOpcode:
            {
                INIT_BYTECODE(SignedRightShift);
                ESIR* signedRightShiftIR = SignedRightShiftIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(signedRightShiftIR);
                NEXT_BYTECODE(SignedRightShift);
                break;
            }
        case UnsignedRightShiftOpcode:
            {
                INIT_BYTECODE(UnsignedRightShift);
                ESIR* unsignedRightShiftIR = UnsignedRightShiftIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(unsignedRightShiftIR);
                NEXT_BYTECODE(UnsignedRightShift);
                break;
            }
        case LessThanOpcode:
            {
                INIT_BYTECODE(LessThan);
                ESIR* lessThanIR = LessThanIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(lessThanIR);
                NEXT_BYTECODE(LessThan);
                break;
            }
        case LessThanOrEqualOpcode:
            {
                INIT_BYTECODE(LessThanOrEqual);
                ESIR* lessThanOrEqualIR = LessThanOrEqualIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(lessThanOrEqualIR);
                NEXT_BYTECODE(LessThanOrEqual);
                break;
            }
        case GreaterThanOpcode:
            {
                INIT_BYTECODE(GreaterThan);
                ESIR* lessThanIR = GreaterThanIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(lessThanIR);
                NEXT_BYTECODE(GreaterThan);
                break;
            }
        case GreaterThanOrEqualOpcode:
            {
                INIT_BYTECODE(GreaterThanOrEqual);
                ESIR* lessThanOrEqualIR = GreaterThanOrEqualIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
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
                ESIR* genericPlusIR = GenericPlusIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(genericPlusIR);
                NEXT_BYTECODE(Plus);
                break;
            }
        case MinusOpcode:
            {
                INIT_BYTECODE(Minus);
                ESIR* minusIR = MinusIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(minusIR);
                NEXT_BYTECODE(Minus);
                break;
            }
        case MultiplyOpcode:
            {
                INIT_BYTECODE(Multiply);
                ESIR* genericMultiplyIR = GenericMultiplyIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(genericMultiplyIR);
                NEXT_BYTECODE(Multiply);
                break;
            }
        case DivisionOpcode:
            {
                INIT_BYTECODE(Division);
                ESIR* genericDivisionIR = GenericDivisionIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
                currentBlock->push(genericDivisionIR);
                NEXT_BYTECODE(Division);
                break;
            }
        case ModOpcode:
            {
                INIT_BYTECODE(Mod);
                ESIR* genericModIR = GenericModIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1]);
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
                ESIR* bitwiseNotIR = BitwiseNotIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(bitwiseNotIR);
                NEXT_BYTECODE(BitwiseNot);
                break;
            }
        case LogicalNotOpcode:
            {
                INIT_BYTECODE(LogicalNot);
                ESIR* logicalNotIR = LogicalNotIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(logicalNotIR);
                NEXT_BYTECODE(LogicalNot);
                break;
            }
        case UnaryMinusOpcode:
            {
                INIT_BYTECODE(UnaryMinus);
                ESIR* UnaryMinusIR = UnaryMinusIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(UnaryMinusIR);
                NEXT_BYTECODE(UnaryMinus);
                break;
            }
        case UnaryPlusOpcode:
            goto unsupported;
            NEXT_BYTECODE(UnaryPlus);
            break;
        case UnaryTypeOfOpcode:
            {
                INIT_BYTECODE(UnaryTypeOf);
                ESIR* typeOfIR = TypeOfIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(typeOfIR);
                NEXT_BYTECODE(UnaryTypeOf);
                break;
            }
        case UnaryDeleteOpcode:
            goto unsupported;
            NEXT_BYTECODE(UnaryDelete);
            break;
        case ToNumberOpcode:
            {
                INIT_BYTECODE(ToNumber);
                ESIR* toNumberIR = ToNumberIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(toNumberIR);
                NEXT_BYTECODE(ToNumber);
                break;
            }
        case IncrementOpcode:
            {
                INIT_BYTECODE(Increment);
                ESIR* incrementIR = IncrementIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(incrementIR);
                NEXT_BYTECODE(Increment);
                break;
            }
        case DecrementOpcode:
            {
                INIT_BYTECODE(Decrement);
                ESIR* decrementIR = DecrementIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(decrementIR);
                NEXT_BYTECODE(Decrement);
                break;
            }
        case CreateObjectOpcode:
            {
                INIT_BYTECODE(CreateObject);
                CreateObjectIR* createObjectIR = CreateObjectIR::create(extraData->m_targetIndex0, bytecode->m_keyCount);
                currentBlock->push(createObjectIR);
                NEXT_BYTECODE(CreateObject);
                break;
            }
        case CreateArrayOpcode:
            {
                INIT_BYTECODE(CreateArray);
                CreateArrayIR* createArrayIR = CreateArrayIR::create(extraData->m_targetIndex0, bytecode->m_keyCount);
                Type t = graph->getOperandType(extraData->m_targetIndex0);
                currentBlock->push(createArrayIR);
                NEXT_BYTECODE(CreateArray);
                break;
            }
        case InitObjectOpcode:
            {
                INIT_BYTECODE(InitObject);
                Type objectType = graph->getOperandType(extraData->m_sourceIndexes[0]);
                ESIR* initObjectIR;
                if (objectType.isArrayObjectType())
                    initObjectIR = InitArrayObjectIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1], extraData->m_sourceIndexes[2]);
                else
                    initObjectIR = InitObjectIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1], extraData->m_sourceIndexes[2]);
                currentBlock->push(initObjectIR);
                NEXT_BYTECODE(InitObject);
                break;
            }
        case GetObjectOpcode:
        case GetObjectAndPushObjectOpcode:
        case GetObjectWithPeekingOpcode:
            {
                GetObject* bytecode = (GetObject*)currentCode;
                ByteCodeExtraData* extraData = &codeBlock->m_extraData[bytecodeCounter];
                ASSERT(codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount < 3);
                if (codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount == 1)
                    graph->setOperandStackPos(extraData->m_targetIndex0, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
                else if (codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount == 2) {
                    currentBlock->push(MoveIR::create(extraData->m_targetIndex1, extraData->m_sourceIndexes[0]));
                    graph->setOperandStackPos(extraData->m_targetIndex0, codeBlock->m_extraData[bytecodeCounter].m_baseRegisterIndex);
                    graph->setOperandStackPos(extraData->m_targetIndex1, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
                } else {
                    RELEASE_ASSERT_NOT_REACHED();
                }
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                GetObjectIR* getObjectIR = GetObjectIR::create(extraData->m_targetIndex0, extraData->m_targetIndex1, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1], bytecode);
                currentBlock->push(getObjectIR);
                NEXT_BYTECODE(GetObject);
                break;
            }
        case GetObjectPreComputedCaseOpcode:
        case GetObjectPreComputedCaseAndPushObjectOpcode:
        case GetObjectWithPeekingPreComputedCaseOpcode:
            {
                GetObjectPreComputedCase* bytecode = (GetObjectPreComputedCase*)currentCode;
                ByteCodeExtraData* extraData = &codeBlock->m_extraData[bytecodeCounter];
                ASSERT(codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount < 3);
                if (codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount == 1) {
                    graph->setOperandStackPos(extraData->m_targetIndex0, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
                } else if (codeBlock->m_extraData[bytecodeCounter].m_registerIncrementCount == 2) {
                    currentBlock->push(MoveIR::create(extraData->m_targetIndex1, extraData->m_sourceIndexes[0]));
                    graph->setOperandStackPos(extraData->m_targetIndex0, codeBlock->m_extraData[bytecodeCounter].m_baseRegisterIndex);
                    graph->setOperandStackPos(extraData->m_targetIndex1, codeBlock->m_extraData[bytecodeCounter + 1].m_baseRegisterIndex);
                } else {
                    RELEASE_ASSERT_NOT_REACHED();
                }
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                GetObjectPreComputedIR* getObjectPreComputedIR = GetObjectPreComputedIR::create(extraData->m_targetIndex0, extraData->m_targetIndex1, extraData->m_sourceIndexes[0],
                    bytecode);
                currentBlock->push(getObjectPreComputedIR);
                NEXT_BYTECODE(GetObjectPreComputedCase);
                break;
            }
        case CreateFunctionOpcode:
            {
                INIT_BYTECODE(CreateFunction);
                CreateFunctionIR* createFunctionIR = CreateFunctionIR::create(extraData->m_targetIndex0, bytecode);
                currentBlock->push(createFunctionIR);
                NEXT_BYTECODE(CreateFunction);
                break;
            }
        case CallFunctionOpcode:
            {
                INIT_BYTECODE(CallFunction);
                int calleeIndex = extraData->m_sourceIndexes[0];
                int receiverIndex = -1;
                int argumentCount = extraData->m_sourceIndexes.size() - 1;
                int* argumentIndexes = (int*) alloca(sizeof(int) * argumentCount);
                for (int i = 0 ; i < argumentCount ; i ++)
                    argumentIndexes[i] = extraData->m_sourceIndexes[i + 1];
                CallJSIR* callJSIR = CallJSIR::create(extraData->m_targetIndex0, calleeIndex, receiverIndex, argumentCount, argumentIndexes);
                currentBlock->push(callJSIR);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                NEXT_BYTECODE(CallFunction);
                break;
            }
        case CallFunctionWithReceiverOpcode:
            {
                INIT_BYTECODE(CallFunctionWithReceiver);
                int calleeIndex = extraData->m_sourceIndexes[0];
                int receiverIndex = extraData->m_sourceIndexes[1];
                int argumentCount = extraData->m_sourceIndexes.size() - 2;
                int* argumentIndexes = (int*) alloca(sizeof(int) * argumentCount);
                for (int i = 0; i < argumentCount; i ++)
                    argumentIndexes[i] = extraData->m_sourceIndexes[i + 2];
                CallJSIR* callJSIR = CallJSIR::create(extraData->m_targetIndex0, calleeIndex, receiverIndex, argumentCount, argumentIndexes);
                currentBlock->push(callJSIR);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                NEXT_BYTECODE(CallFunctionWithReceiver);
                break;
            }
        case NewFunctionCallOpcode:
            {
                INIT_BYTECODE(NewFunctionCall);
                int calleeIndex = extraData->m_sourceIndexes[0];
                int receiverIndex = -1;
                int argumentCount = extraData->m_sourceIndexes.size() - 1;
                int* argumentIndexes = (int*) alloca(sizeof(int) * argumentCount);
                for (int i = 0; i < argumentCount; i++)
                    argumentIndexes[i] = extraData->m_sourceIndexes[i + 1];
                CallNewJSIR* callNewJSIR = CallNewJSIR::create(extraData->m_targetIndex0, calleeIndex, receiverIndex, argumentCount, argumentIndexes);
                currentBlock->push(callNewJSIR);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                NEXT_BYTECODE(NewFunctionCall);
                break;
            }
        case CallEvalFunctionOpcode:
            {
                INIT_BYTECODE(CallEvalFunction);
                int argumentCount = extraData->m_sourceIndexes.size();
                int* argumentIndexes = (int*) alloca(sizeof(int) * argumentCount);
                for (int i = 0; i < argumentCount; i++)
                    argumentIndexes[i] = extraData->m_sourceIndexes[i];
                CallEvalIR* callEvalIR = CallEvalIR::create(extraData->m_targetIndex0, argumentCount, argumentIndexes);
                currentBlock->push(callEvalIR);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                NEXT_BYTECODE(CallEvalFunction);
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
                ReturnWithValueIR* returnWithValueIR = ReturnWithValueIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
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

                JumpIR* jumpIR = JumpIR::create(extraData->m_targetIndex0, targetBlock);
                currentBlock->push(jumpIR);
                basicBlockMapping[bytecode->m_jumpPosition] = targetBlock;

                if (bytecode->m_jumpPosition < idx) {
                    if (backWordJumpMapping.find(bytecode->m_jumpPosition) != backWordJumpMapping.end()) {
                        // FIXME: fno-rtti : can't be ir any other than GetEnumerableObjectIR??
                        // GetEnumerablObjectIR* getEnumerablObjectIR = dynamic_cast<GetEnumerablObjectIR*>(backWordJumpMapping[bytecode->m_jumpPosition]);
                        GetEnumerablObjectDataIR* getEnumerablObjectDataIR = (GetEnumerablObjectDataIR*)backWordJumpMapping[bytecode->m_jumpPosition];
                        getEnumerablObjectDataIR->setJumpIR(jumpIR);
                    }
                }

                NEXT_BYTECODE(Jump);
                break;
            }
        case JumpIfTopOfStackValueIsFalseOpcode:
            {
                INIT_BYTECODE(JumpIfTopOfStackValueIsFalse);

                std::map<int, ESBasicBlock*>::iterator findIter;
                ESBasicBlock* trueBlock, *falseBlock;
                if ((findIter = basicBlockMapping.find(idx + sizeof(JumpIfTopOfStackValueIsFalse))) != basicBlockMapping.end()) {
                    trueBlock = basicBlockMapping[idx + sizeof(JumpIfTopOfStackValueIsFalse)];
                } else {
                    trueBlock = ESBasicBlock::create(graph, currentBlock);
                    basicBlockMapping[idx + sizeof(JumpIfTopOfStackValueIsFalse)] = trueBlock;
                }

                if ((findIter = basicBlockMapping.find(bytecode->m_jumpPosition)) != basicBlockMapping.end()) {
                    falseBlock = basicBlockMapping[bytecode->m_jumpPosition];
                } else {
                    falseBlock = ESBasicBlock::create(graph, currentBlock, true);
                    basicBlockMapping[bytecode->m_jumpPosition] = falseBlock;
                }

                BranchIR* branchIR = BranchIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], trueBlock, falseBlock);
                currentBlock->push(branchIR);

                NEXT_BYTECODE(JumpIfTopOfStackValueIsFalse);
                break;
            }
        case JumpIfTopOfStackValueIsTrueOpcode:
            {
                INIT_BYTECODE(JumpIfTopOfStackValueIsTrue);

                std::map<int, ESBasicBlock*>::iterator findIter;
                ESBasicBlock* trueBlock, *falseBlock;
                if ((findIter = basicBlockMapping.find(idx + sizeof(JumpIfTopOfStackValueIsTrue))) != basicBlockMapping.end()) {
                    falseBlock = basicBlockMapping[idx + sizeof(JumpIfTopOfStackValueIsTrue)];
                } else {
                    falseBlock = ESBasicBlock::create(graph, currentBlock);
                    basicBlockMapping[idx + sizeof(JumpIfTopOfStackValueIsTrue)] = falseBlock;
                }

                if ((findIter = basicBlockMapping.find(bytecode->m_jumpPosition)) != basicBlockMapping.end()) {
                    trueBlock = basicBlockMapping[bytecode->m_jumpPosition];
                } else {
                    trueBlock = ESBasicBlock::create(graph, currentBlock, true);
                    basicBlockMapping[bytecode->m_jumpPosition] = trueBlock;
                }

                BranchIR* branchIR = BranchIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], trueBlock, falseBlock);
                currentBlock->push(branchIR);

                NEXT_BYTECODE(JumpIfTopOfStackValueIsTrue);
                break;
            }
        case JumpIfTopOfStackValueIsFalseWithPeekingOpcode:
            {
                INIT_BYTECODE(JumpIfTopOfStackValueIsFalseWithPeeking);

                std::map<int, ESBasicBlock*>::iterator findIter;
                ESBasicBlock* trueBlock, *falseBlock;
                if ((findIter = basicBlockMapping.find(idx + sizeof(JumpIfTopOfStackValueIsFalseWithPeeking))) != basicBlockMapping.end()) {
                    trueBlock = basicBlockMapping[idx + sizeof(JumpIfTopOfStackValueIsFalseWithPeeking)];
                } else {
                    trueBlock = ESBasicBlock::create(graph, currentBlock);
                    basicBlockMapping[idx + sizeof(JumpIfTopOfStackValueIsFalseWithPeeking)] = trueBlock;
                }

                if ((findIter = basicBlockMapping.find(bytecode->m_jumpPosition)) != basicBlockMapping.end()) {
                    falseBlock = basicBlockMapping[bytecode->m_jumpPosition];
                } else {
                    falseBlock = ESBasicBlock::create(graph, currentBlock, true);
                    basicBlockMapping[bytecode->m_jumpPosition] = falseBlock;
                }

                BranchIR* branchIR = BranchIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], trueBlock, falseBlock);
                currentBlock->push(branchIR);

                NEXT_BYTECODE(JumpIfTopOfStackValueIsFalseWithPeeking);
                break;
            }
        case JumpIfTopOfStackValueIsTrueWithPeekingOpcode:
            {
                INIT_BYTECODE(JumpIfTopOfStackValueIsTrueWithPeeking);

                std::map<int, ESBasicBlock*>::iterator findIter;
                ESBasicBlock* trueBlock, *falseBlock;
                if ((findIter = basicBlockMapping.find(idx + sizeof(JumpIfTopOfStackValueIsTrueWithPeeking))) != basicBlockMapping.end()) {
                    falseBlock = basicBlockMapping[idx + sizeof(JumpIfTopOfStackValueIsTrueWithPeeking)];
                } else {
                    falseBlock = ESBasicBlock::create(graph, currentBlock);
                    basicBlockMapping[idx + sizeof(JumpIfTopOfStackValueIsTrueWithPeeking)] = falseBlock;
                }

                if ((findIter = basicBlockMapping.find(bytecode->m_jumpPosition)) != basicBlockMapping.end()) {
                    trueBlock = basicBlockMapping[bytecode->m_jumpPosition];
                } else {
                    trueBlock = ESBasicBlock::create(graph, currentBlock, true);
                    basicBlockMapping[bytecode->m_jumpPosition] = trueBlock;
                }

                BranchIR* branchIR = BranchIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], trueBlock, falseBlock);
                currentBlock->push(branchIR);

                NEXT_BYTECODE(JumpIfTopOfStackValueIsTrueWithPeeking);
                break;
            }
        case JumpAndPopIfTopOfStackValueIsTrueOpcode:
            {
                INIT_BYTECODE(JumpAndPopIfTopOfStackValueIsTrue);

                std::map<int, ESBasicBlock*>::iterator findIter;
                ESBasicBlock* trueBlock, *falseBlock;
                if ((findIter = basicBlockMapping.find(idx + sizeof(JumpAndPopIfTopOfStackValueIsTrue))) != basicBlockMapping.end()) {
                    falseBlock = basicBlockMapping[idx + sizeof(JumpAndPopIfTopOfStackValueIsTrue)];
                } else {
                    falseBlock = ESBasicBlock::create(graph, currentBlock);
                    basicBlockMapping[idx + sizeof(JumpAndPopIfTopOfStackValueIsTrue)] = falseBlock;
                }

                if ((findIter = basicBlockMapping.find(bytecode->m_jumpPosition)) != basicBlockMapping.end()) {
                    trueBlock = basicBlockMapping[bytecode->m_jumpPosition];
                } else {
                    trueBlock = ESBasicBlock::create(graph, currentBlock, true);
                    basicBlockMapping[bytecode->m_jumpPosition] = trueBlock;
                }

                BranchIR* branchIR = BranchIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], trueBlock, falseBlock);
                currentBlock->push(branchIR);
                NEXT_BYTECODE(JumpAndPopIfTopOfStackValueIsTrue);
                break;
            }
        case DuplicateTopOfStackValueOpcode:
            {
                INIT_BYTECODE(DuplicateTopOfStackValue);
                MoveIR* moveIR = MoveIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
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
            {
                INIT_BYTECODE(Throw);
                ThrowIR* throwIR = ThrowIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(throwIR);
                NEXT_BYTECODE(Throw);
                break;
            }
        case AllocPhiOpcode:
            {
                INIT_BYTECODE(AllocPhi);
                AllocPhiIR* allocPhiIR = AllocPhiIR::create(extraData->m_targetIndex0);
                currentBlock->push(allocPhiIR);
                NEXT_BYTECODE(AllocPhi);
                break;
            }
        case StorePhiOpcode:
            {
                INIT_BYTECODE(StorePhi);
                StorePhiIR* storePhiIR = StorePhiIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[1], extraData->m_sourceIndexes[0]);
                currentBlock->push(storePhiIR);
                NEXT_BYTECODE(StorePhi);
                break;
            }
        case LoadPhiOpcode:
            {
                INIT_BYTECODE(LoadPhi);
                LoadPhiIR* loadPhiIR = LoadPhiIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0], extraData->m_sourceIndexes[1], extraData->m_sourceIndexes[2]);
                currentBlock->push(loadPhiIR);
                NEXT_BYTECODE(LoadPhi);
                break;
            }
        case ThisOpcode:
            {
                INIT_BYTECODE(This);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                GetThisIR* getThisIR = GetThisIR::create(extraData->m_targetIndex0);
                currentBlock->push(getThisIR);
                NEXT_BYTECODE(This);
                break;
            }
        case EnumerateObjectOpcode:
            {
                INIT_BYTECODE(EnumerateObject);
                GetEnumerablObjectDataIR* getEnumerablObjectDataIR = GetEnumerablObjectDataIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(getEnumerablObjectDataIR);
                backWordJumpMapping[idx + sizeof(EnumerateObject) + sizeof(LoopStart)] = getEnumerablObjectDataIR;
                NEXT_BYTECODE(EnumerateObject);
                break;
            }
        case CheckIfKeyIsLastOpcode:
            {
                INIT_BYTECODE(CheckIfKeyIsLast);
                CheckIfKeyIsLastIR* checkIfKeyIsLastIR = CheckIfKeyIsLastIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(checkIfKeyIsLastIR);
                NEXT_BYTECODE(CheckIfKeyIsLast);
                break;
            }
        case EnumerateObjectKeyOpcode:
            {
                INIT_BYTECODE(EnumerateObjectKey);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(extraData->m_targetIndex0, bytecode->m_profile.getType());
                GetEnumerateKeyIR* getEnumerateKeyIR = GetEnumerateKeyIR::create(extraData->m_targetIndex0, extraData->m_sourceIndexes[0]);
                currentBlock->push(getEnumerateKeyIR);
                NEXT_BYTECODE(EnumerateObjectKey);
                break;
            }
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
    GC_enable();
    return nullptr;
}

}}
#endif
