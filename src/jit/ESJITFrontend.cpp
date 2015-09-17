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

#if 0
ConstantIR* ConstantIR::s_undefined = ConstantIR::create(ESValue());
#ifdef ESCARGOT_32
COMPILE_ASSERT(false, "define the mask value");
#else
ConstantIR* ConstantIR::s_int32Mask = ConstantIR::create(ESValue(0xffff000000000000));
#endif
ConstantIR* ConstantIR::s_zero = ConstantIR::create(ESValue(0));
#endif

ESIR* typeCheck(ESBasicBlock* block, ESIR* ir, Type type, int bytecodeIndex)
{
#if 0
    ESIR* mask = ConstantIR::getMask(type);
    block->push(mask);
    ESIR* bitwiseAnd = BitwiseAndIR::create(mask, ir);
    block->push(bitwiseAnd);
    ESIR* zero = ConstantIR::s_zero;
    block->push(zero);
    ESIR* compare = EqualIR::create(bitwiseAnd, zero);
    block->push(compare);
    ESIR* returnIndex = ConstantIR::create(ESValue(bytecodeIndex));
    block->push(returnIndex);
    ESIR* ret = ReturnIR::create(returnIndex);
#endif

    return ir;
}

ESGraph* generateIRFromByteCode(CodeBlock* codeBlock)
{
#ifndef NDEBUG
    dumpBytecode(codeBlock);
#endif

    ESGraph* graph = ESGraph::create(codeBlock);

    size_t idx = 0;
    char* code = codeBlock->m_code.data();

    std::map<int, ESBasicBlock*> basicBlockMapping;

    ESBasicBlock *entryBlock = ESBasicBlock::create(graph);
    basicBlockMapping[idx] = entryBlock;
    ESBasicBlock* currentBlock = entryBlock;

    while(idx < codeBlock->m_code.size()) {
        ByteCode* currentCode = (ByteCode *)(&code[idx]);

        Opcode opcode = Opcode::OpcodeKindEnd;
        for(int i = 0; i < Opcode::OpcodeKindEnd; i ++) {
            if((ESVMInstance::currentInstance()->opcodeTable())->m_table[i] == currentCode->m_opcode) {
                opcode = (Opcode)i;
                break;
            }
        }

        // Update BasicBlock information 
        // TODO: find a better way to this (using AST, write information to bytecode..)
        if (ESBasicBlock* generatedBlock = basicBlockMapping[idx]) {
            if (currentBlock != generatedBlock && !currentBlock->endsWithJumpOrBranch()) {
                currentBlock->addChild(generatedBlock);
                generatedBlock->addParent(currentBlock);
            }
            currentBlock = generatedBlock;
        }
        //printf("parse idx %lu with BasicBlock %lu\n", idx, currentBlock->index());

#define INIT_BYTECODE(ByteCode) ByteCode* bytecode = (ByteCode*)currentCode;
#define NEXT_BYTECODE(ByteCode) idx += sizeof(ByteCode);
        switch(opcode) {
        case PushOpcode:
        {
            INIT_BYTECODE(Push);
            ESIR* literal;
            if (bytecode->m_value.isInt32())
                literal = ConstantIntIR::create(bytecode->m_targetIndex, bytecode->m_value.asInt32());
            else
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
            if (bytecode->m_index < codeBlock->m_params.size()) {
                ESIR* getArgument = GetArgumentIR::create(bytecode->m_targetIndex, bytecode->m_index);
                currentBlock->push(getArgument);
            } else {
                ESIR* getVar = GetVarIR::create(bytecode->m_targetIndex, bytecode->m_index);
                currentBlock->push(getVar);
            }
            NEXT_BYTECODE(GetByIndex);
            break;
        }
        case GetByIndexWithActivationOpcode:
            NEXT_BYTECODE(GetByIndexWithActivation);
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
            NEXT_BYTECODE(BitwiseAnd);
            break;
        case BitwiseOrOpcode:
            NEXT_BYTECODE(BitwiseOr);
            break;
        case BitwiseXorOpcode:
            NEXT_BYTECODE(BitwiseXor);
            break;
        case LeftShiftOpcode:
            NEXT_BYTECODE(LeftShift);
            break;
        case SignedRightShiftOpcode:
            NEXT_BYTECODE(SignedRightShift);
            break;
        case UnsignedRightShiftOpcode:
            NEXT_BYTECODE(UnsignedRightShift);
            break;
        case LessThanOpcode:
        {
            INIT_BYTECODE(LessThan);
            ESIR* lessThanIR = LessThanIR::create(bytecode->m_targetIndex, bytecode->m_leftIndex, bytecode->m_rightIndex);
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
#if 0
            // TODO
            // 1. if both arguments have number type then append StringPlus
            // 2. else if either one of arguments has string type then append NumberPlus
            // 3. else append general Plus
#endif
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
        case CallFunctionOpcode:
            NEXT_BYTECODE(CallFunction);
            break;
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
            ReturnWithValueIR* returnWithValueIR = ReturnWithValueIR::create(bytecode->m_targetIndex, bytecode->m_returnIndex);
            currentBlock->push(returnWithValueIR);
            NEXT_BYTECODE(ReturnFunctionWithValue);
            break;
        }
        case JumpOpcode:
        {
            INIT_BYTECODE(Jump);
            ESBasicBlock* targetBlock = ESBasicBlock::create(graph, currentBlock);
            JumpIR* jumpIR = JumpIR::create(bytecode->m_targetIndex, targetBlock);
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

            BranchIR* branchIR = BranchIR::create(bytecode->m_targetIndex, bytecode->m_conditionIndex, trueBlock, falseBlock);
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
    return graph;
}

}}
