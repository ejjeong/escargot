#include "Escargot.h"
#include "ESJITFrontend.h"

#include "ESGraph.h"
#include "ESIR.h"

#include "bytecode/ByteCode.h"

namespace escargot {
namespace ESJIT {

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

ESGraph* handMade()
{
    ESGraph* cfg = new ESGraph();

    // [0] a: int32
    // [1] b: int32
    // function boo(a) {
    //     var b;
    //     if (a) b = a+1;
    //     else   b = 0xff;
    //     return b;
    // }

    ESBasicBlock *entryBlock = new ESBasicBlock();
    cfg->push(entryBlock);

#if 0
    // IdentifierFastCaseNode (get a)
    ESIR* undefined = ConstantIR::s_undefined;
    entryBlock->push(undefined);
    ESIR* setB = SetVar::create(1, undefined);
    entryBlock->push(setB);
    ESIR* getA = GetVarIR::create(0);
    entryBlock->push(getA);
    typeCheck(entryBlock, getA, Type::Int32, /* bytecode index to restart execution*/4);
#endif
#if 0
    // IfStatementNode (test if true/false)
    entryBlock->push(Equal::create());
    ESBasicBlock* trueBlock = new ESBasicBlock(entryBlock);
    ESBasicBlock* falseBlock = new ESBasicBlock(entryBlock);
    entryBlock->push(Branch::create(trueBlock, falseBlock));

    cfg->addBasicBlock(trueBlock);
    cfg->addBasicBlock(falseBlock);

    // ReturnStatementNode ( return a; )
    trueBlock->push(Return::create());

    // ReturnStatementNode ( return 0xff; ) , LiteralNode ( 0xff; )
    falseBlock->push(ESIR::IntConstant(0xff));
    falseBlock->push(ESIR::Return::create());

    entryBlock->push(ESIR::SetVar::create(1));
#endif

    return cfg;
}

template <typename CodeType>
ALWAYS_INLINE void setupForNextCode(size_t& programCounter, int& sp)
{
    programCounter += sizeof (CodeType);
    // FIXME: do sth with sp automatically (need some pre-defined stack push/pop counts)
}

ESGraph* generateIRFromByteCode(CodeBlock* codeBlock)
{
#if 0
    return handMade();
#else
    ESGraph* cfg = new ESGraph();

    size_t programCounter = 0;
    char* code = codeBlock->m_code.data();
    int sp = 0;
    int tmp = 0;

    ESBasicBlock *currentBlock = new ESBasicBlock();
    cfg->push(currentBlock);

    while(1) {
        ByteCode* currentCode = (ByteCode *)(&code[programCounter]);
        switch(currentCode->m_opcode) {
        case PushOpcode:
        {
            Push* pushCode = (Push*)currentCode;
            ESIR* literal = ConstantIR::create(tmp, pushCode->m_value);
            currentBlock->push(literal);
            setupForNextCode<Push>(programCounter, sp);
            break;
        }
        case PopExpressionStatementOpcode:
            setupForNextCode<PopExpressionStatement>(programCounter, sp);
            break;
        case PopOpcode:
            setupForNextCode<Pop>(programCounter, sp);
            break;
        case GetByIdOpcode:
            setupForNextCode<GetById>(programCounter, sp);
            break;
        case GetByIndexOpcode:
            setupForNextCode<GetByIndex>(programCounter, sp);
            break;
        case GetByIndexWithActivationOpcode:
            setupForNextCode<GetByIndexWithActivation>(programCounter, sp);
            break;
        case ResolveAddressByIdOpcode:
            setupForNextCode<ResolveAddressById>(programCounter, sp);
            break;
        case ResolveAddressByIndexOpcode:
            setupForNextCode<ResolveAddressByIndex>(programCounter, sp);
            break;
        case ResolveAddressByIndexWithActivationOpcode:
            setupForNextCode<ResolveAddressByIndexWithActivation>(programCounter, sp);
            break;
        case ResolveAddressInObjectOpcode:
            setupForNextCode<ResolveAddressInObject>(programCounter, sp);
            break;
        case PutOpcode:
            setupForNextCode<Put>(programCounter, sp);
            break;
        case PutReverseStackOpcode:
            setupForNextCode<PutReverseStack>(programCounter, sp);
            break;
        case CreateBindingOpcode:
            setupForNextCode<CreateBinding>(programCounter, sp);
            break;
        case EqualOpcode:
            setupForNextCode<Equal>(programCounter, sp);
            break;
        case NotEqualOpcode:
            setupForNextCode<NotEqual>(programCounter, sp);
            break;
        case StrictEqualOpcode:
            setupForNextCode<StrictEqual>(programCounter, sp);
            break;
        case NotStrictEqualOpcode:
            setupForNextCode<NotStrictEqual>(programCounter, sp);
            break;
        case BitwiseAndOpcode:
            setupForNextCode<BitwiseAnd>(programCounter, sp);
            break;
        case BitwiseOrOpcode:
            setupForNextCode<BitwiseOr>(programCounter, sp);
            break;
        case BitwiseXorOpcode:
            setupForNextCode<BitwiseXor>(programCounter, sp);
            break;
        case LeftShiftOpcode:
            setupForNextCode<LeftShift>(programCounter, sp);
            break;
        case SignedRightShiftOpcode:
            setupForNextCode<SignedRightShift>(programCounter, sp);
            break;
        case UnsignedRightShiftOpcode:
            setupForNextCode<UnsignedRightShift>(programCounter, sp);
            break;
        case LessThanOpcode:
            setupForNextCode<LessThan>(programCounter, sp);
            break;
        case LessThanOrEqualOpcode:
            setupForNextCode<LessThanOrEqual>(programCounter, sp);
            break;
        case GreaterThanOpcode:
            setupForNextCode<GreaterThan>(programCounter, sp);
            break;
        case GreaterThanOrEqualOpcode:
            setupForNextCode<GreaterThanOrEqual>(programCounter, sp);
            break;
        case PlusOpcode:
        {
#if 0
            // TODO
            // 1. if both arguments have number type then append StringPlus
            // 2. else if either one of arguments has string type then append NumberPlus
            // 3. else append general Plus
            ESIR* plus = GenericPlusIR::create(sp-1, sp-2);
            currentBlock->push(plus, sp-2);
            sp--;
#endif
            setupForNextCode<Plus>(programCounter, sp);
            break;
        }
        case MinusOpcode:
            setupForNextCode<Minus>(programCounter, sp);
            break;
        case MultiplyOpcode:
            setupForNextCode<Multiply>(programCounter, sp);
            break;
        case DivisionOpcode:
            setupForNextCode<Division>(programCounter, sp);
            break;
        case ModOpcode:
            setupForNextCode<Mod>(programCounter, sp);
            break;
        case CreateObjectOpcode:
            setupForNextCode<CreateObject>(programCounter, sp);
            break;
        case CreateArrayOpcode:
            setupForNextCode<CreateArray>(programCounter, sp);
            break;
        case SetObjectOpcode:
            setupForNextCode<SetObject>(programCounter, sp);
            break;
        case GetObjectOpcode:
            setupForNextCode<GetObject>(programCounter, sp);
            break;
        case JumpOpcode:
            setupForNextCode<Jump>(programCounter, sp);
            break;
        case JumpIfTopOfStackValueIsFalseOpcode:
            setupForNextCode<JumpIfTopOfStackValueIsFalse>(programCounter, sp);
            break;
        case JumpIfTopOfStackValueIsTrueOpcode:
            setupForNextCode<JumpIfTopOfStackValueIsTrue>(programCounter, sp);
            break;
        case JumpIfTopOfStackValueIsFalseWithPeekingOpcode:
            setupForNextCode<JumpIfTopOfStackValueIsFalseWithPeeking>(programCounter, sp);
            break;
        case JumpIfTopOfStackValueIsTrueWithPeekingOpcode:
            setupForNextCode<JumpIfTopOfStackValueIsTrueWithPeeking>(programCounter, sp);
            break;
            /*
        case CallOpcode:
            setupForNextCode<Call>(programCounter, sp);
            break;
            */
        case DuplicateTopOfStackValueOpcode:
            setupForNextCode<DuplicateTopOfStackValue>(programCounter, sp);
            break;
        case ThrowOpcode:
            setupForNextCode<Throw>(programCounter, sp);
            break;
        case EndOpcode:
            goto postprocess;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
postprocess:
    return cfg;
#endif
}

}}
