#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITFrontend.h"

#include "ESGraph.h"
#include "ESIR.h"

#include "bytecode/ByteCode.h"

namespace escargot {
namespace ESJIT {

#define DECLARE_BYTECODE_LENGTH(bytecode) const int bytecode##Length = sizeof(bytecode);
    FOR_EACH_BYTECODE_OP(DECLARE_BYTECODE_LENGTH)
#undef DECLARE_BYTECODE_LENGTH

ESGraph* generateIRFromByteCode(CodeBlock* codeBlock)
{
#ifndef NDEBUG
    dumpBytecode(codeBlock);
#endif

    ESGraph* graph = ESGraph::create(codeBlock);

    size_t idx = 0;
    size_t bytecodeCounter = 0;
    char* code = codeBlock->m_code.data();

    std::map<int, ESBasicBlock*> basicBlockMapping;

    ESBasicBlock *entryBlock = ESBasicBlock::create(graph);
    basicBlockMapping[idx] = entryBlock;
    ESBasicBlock* currentBlock = entryBlock;

    while(idx < codeBlock->m_code.size()) {
        ByteCode* currentCode = (ByteCode *)(&code[idx]);

        // TODO: find a better way to this (e.g. write the size of the bytecode in FOR_EACH_BYTECODE_OP macro)
        Opcode opcode = Opcode::OpcodeKindEnd;
        for(int i = 0; i < Opcode::OpcodeKindEnd; i ++) {
            if((ESVMInstance::currentInstance()->opcodeTable())->m_table[i] == currentCode->m_opcode) {
                opcode = (Opcode)i;
                break;
            }
        }

        // Update BasicBlock information 
        // TODO: find a better way to this (e.g. using AST, write information to bytecode..)
        if (ESBasicBlock* generatedBlock = basicBlockMapping[idx]) {
            if (currentBlock != generatedBlock && !currentBlock->endsWithJumpOrBranch()) {
                currentBlock->addChild(generatedBlock);
                generatedBlock->addParent(currentBlock);
            }
            currentBlock = generatedBlock;
        }
        // printf("parse idx %lu with BasicBlock %lu\n", idx, currentBlock->index());

#define INIT_BYTECODE(ByteCode) \
            ByteCode* bytecode = (ByteCode*)currentCode; \
            SSAIndex* ssaIndex = codeBlock->getSSAIndex(bytecodeCounter);
#define NEXT_BYTECODE(ByteCode) \
            idx += sizeof(ByteCode); \
            bytecodeCounter++;
        switch(opcode) {
        case PushOpcode:
        {
            INIT_BYTECODE(Push);
            ESIR* literal;
            if (bytecode->m_value.isInt32()) {
                literal = ConstantIntIR::create(ssaIndex->m_targetIndex, bytecode->m_value.asInt32());
            } else
                RELEASE_ASSERT_NOT_REACHED();
            currentBlock->push(literal);
            NEXT_BYTECODE(Push);
            break;
        }
        case PopExpressionStatementOpcode:
            NEXT_BYTECODE(PopExpressionStatement);
            break;
        case PopOpcode:
            NEXT_BYTECODE(Pop);
            break;
        case GetByIdOpcode:
            NEXT_BYTECODE(GetById);
            break;
        case GetByIndexOpcode:
        {
            INIT_BYTECODE(GetByIndex);
            // TODO: load from local variable should not be a heap load.
            if (bytecode->m_index < codeBlock->m_params.size()) {
                ESIR* getArgument = GetArgumentIR::create(ssaIndex->m_targetIndex, bytecode->m_index);
                currentBlock->push(getArgument);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            } else {
                ESIR* getVar = GetVarIR::create(ssaIndex->m_targetIndex, bytecode->m_index);
                currentBlock->push(getVar);
                bytecode->m_profile.updateProfiledType();
                graph->setOperandType(ssaIndex->m_targetIndex, bytecode->m_profile.getType());
            }
            NEXT_BYTECODE(GetByIndex);
            break;
        }
        case GetByIndexWithActivationOpcode:
            NEXT_BYTECODE(GetByIndexWithActivation);
            break;
        case PutByIdOpcode:
            NEXT_BYTECODE(PutById);
            break;
        case PutByIndexOpcode:
        {
            INIT_BYTECODE(PutByIndex);
            ESIR* setVar = SetVarIR::create(ssaIndex->m_targetIndex, bytecode->m_index, ssaIndex->m_srcIndex1);
            currentBlock->push(setVar);
            NEXT_BYTECODE(PutByIndex);
            break;
        }
        case PutByIndexWithActivationOpcode:
            NEXT_BYTECODE(PutByIndexWithActivation);
            break;
        case PutInObjectOpcode:
            NEXT_BYTECODE(PutInObject);
            break;
        case CreateBindingOpcode:
            NEXT_BYTECODE(CreateBinding);
            break;
        case EqualOpcode:
            NEXT_BYTECODE(Equal);
            break;
        case NotEqualOpcode:
            NEXT_BYTECODE(NotEqual);
            break;
        case StrictEqualOpcode:
            NEXT_BYTECODE(StrictEqual);
            break;
        case NotStrictEqualOpcode:
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
            NEXT_BYTECODE(BitwiseOr);
            break;
        case BitwiseXorOpcode:
            NEXT_BYTECODE(BitwiseXor);
            break;
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
            NEXT_BYTECODE(UnsignedRightShift);
            break;
        case LessThanOpcode:
        {
            INIT_BYTECODE(LessThan);
            ESIR* lessThanIR = LessThanIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(lessThanIR);
            NEXT_BYTECODE(LessThan);
            break;
        }
        case LessThanOrEqualOpcode:
            NEXT_BYTECODE(LessThanOrEqual);
            break;
        case GreaterThanOpcode:
            NEXT_BYTECODE(GreaterThan);
            break;
        case GreaterThanOrEqualOpcode:
            NEXT_BYTECODE(GreaterThanOrEqual);
            break;
        case PlusOpcode:
        {
            // TODO
            // 1. if both arguments have number type then append StringPlus
            // 2. else if either one of arguments has string type then append NumberPlus
            // 3. else append general Plus
            INIT_BYTECODE(Plus);
            ESIR* genericPlusIR = GenericPlusIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, ssaIndex->m_srcIndex2);
            currentBlock->push(genericPlusIR);
            NEXT_BYTECODE(Plus);
            break;
        }
        case MinusOpcode:
            NEXT_BYTECODE(Minus);
            break;
        case MultiplyOpcode:
            NEXT_BYTECODE(Multiply);
            break;
        case DivisionOpcode:
            NEXT_BYTECODE(Division);
            break;
        case ModOpcode:
            NEXT_BYTECODE(Mod);
            break;
        case IncrementOpcode:
        {
            NEXT_BYTECODE(Increment);
            break;
        }
        case DecrementOpcode:
            NEXT_BYTECODE(Decrement);
            break;
        case StringInOpcode:
            NEXT_BYTECODE(StringIn);
            break;
        case BitwiseNotOpcode:
            NEXT_BYTECODE(BitwiseNot);
            break;
        case LogicalNotOpcode:
            NEXT_BYTECODE(LogicalNot);
            break;
        case UnaryMinusOpcode:
            NEXT_BYTECODE(UnaryMinus);
            break;
        case UnaryPlusOpcode:
            NEXT_BYTECODE(UnaryPlus);
            break;
        case UnaryTypeOfOpcode:
            NEXT_BYTECODE(UnaryTypeOf);
            break;
        case UnaryDeleteOpcode:
            NEXT_BYTECODE(UnaryDelete);
            break;
        case ToNumberOpcode:
        {
            INIT_BYTECODE(ToNumber);
            ESIR* toNumberIR = ToNumberIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1);
            currentBlock->push(toNumberIR);
            NEXT_BYTECODE(ToNumber);
            break;
        }
        case CreateObjectOpcode:
            NEXT_BYTECODE(CreateObject);
            break;
        case CreateArrayOpcode:
            NEXT_BYTECODE(CreateArray);
            break;
        case SetObjectOpcode:
            NEXT_BYTECODE(SetObject);
            break;
        case GetObjectOpcode:
            NEXT_BYTECODE(GetObject);
            break;
        case CreateFunctionOpcode:
            NEXT_BYTECODE(ExecuteNativeFunction);
            break;
        case PrepareFunctionCallOpcode:
            NEXT_BYTECODE(PrepareFunctionCall);
            break;
        case PushFunctionCallReceiverOpcode: 
            NEXT_BYTECODE(PushFunctionCallReceiver);
            break;
        case CallFunctionOpcode:
        {
            NEXT_BYTECODE(CallFunction);
            break;
        }
        case NewFunctionCallOpcode:
            NEXT_BYTECODE(NewFunctionCall);
            break;
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
                targetBlock = ESBasicBlock::create(graph, currentBlock);
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
            ESBasicBlock* falseBlock = ESBasicBlock::create(graph, currentBlock);

            BranchIR* branchIR = BranchIR::create(ssaIndex->m_targetIndex, ssaIndex->m_srcIndex1, trueBlock, falseBlock);
            currentBlock->push(branchIR);

            basicBlockMapping[idx + sizeof(JumpIfTopOfStackValueIsFalse)] = trueBlock;
            basicBlockMapping[bytecode->m_jumpPosition] = falseBlock;

            NEXT_BYTECODE(JumpIfTopOfStackValueIsFalse);
            break;
        }
        case JumpIfTopOfStackValueIsTrueOpcode:
            NEXT_BYTECODE(JumpIfTopOfStackValueIsTrue);
            break;
        case JumpIfTopOfStackValueIsFalseWithPeekingOpcode:
            NEXT_BYTECODE(JumpIfTopOfStackValueIsFalseWithPeeking);
            break;
        case JumpIfTopOfStackValueIsTrueWithPeekingOpcode:
            NEXT_BYTECODE(JumpIfTopOfStackValueIsTrueWithPeeking);
            break;
        case DuplicateTopOfStackValueOpcode:
            NEXT_BYTECODE(DuplicateTopOfStackValue);
            break;
        case LoopStartOpcode:
        {
            INIT_BYTECODE(LoopStart);
            ESBasicBlock* loopBlock = ESBasicBlock::create(graph);
            basicBlockMapping[idx + sizeof(LoopStart)] = loopBlock;
            NEXT_BYTECODE(LoopStart);
            break;
        }
        case ThrowOpcode:
            NEXT_BYTECODE(Throw);
            break;
        case EndOpcode:
            goto postprocess;
        default:
            printf("Invalid Opcode %d\n", opcode);
            RELEASE_ASSERT_NOT_REACHED();
        }
#undef INIT_BYTECODE
#undef NEXT_BYTECODE
    }
postprocess:
#ifndef NDEBUG
    graph->dump(std::cout);
#endif
    return graph;
}

}}
#endif
