#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITMiddleend.h"

#include "ESGraph.h"
#include "ESIR.h"

namespace escargot {
namespace ESJIT {

// In interpreter, we only profile the type of 1. the variable loaded from the
// heap, 2. function argument, and 3. return value of function call.
// For other IRs, We should run type inference phase to examine the types.

void ESGraphTypeInference::run(ESGraph* graph)
{
    for (size_t i = 0; i < graph->basicBlockSize(); i++) {
        ESBasicBlock* block = graph->basicBlock(i);
        for (size_t j = 0; j < block->instructionSize(); j++) {
            ESIR* ir = block->instruction(j);
            switch(ir->opcode()) {
            #define INIT_ESIR(opcode) \
                opcode##IR* ir##opcode = static_cast<opcode##IR*>(ir);
            case ESIR::Opcode::Constant:
                graph->setOperandType(ir->targetIndex(), TypeTop);
                break;
            case ESIR::Opcode::ConstantInt:
                graph->setOperandType(ir->targetIndex(), TypeInt32);
                break;
            case ESIR::Opcode::ConstantDouble:
                graph->setOperandType(ir->targetIndex(), TypeDouble);
                break;
            case ESIR::Opcode::ConstantTrue:
            case ESIR::Opcode::ConstantFalse:
                graph->setOperandType(ir->targetIndex(), TypeBoolean);
                break;
            case ESIR::Opcode::ConstantString:
                graph->setOperandType(ir->targetIndex(), TypeString);
                break;
            case ESIR::Opcode::GenericPlus:
            {
                INIT_ESIR(GenericPlus);
                Type leftType = graph->getOperandType(irGenericPlus->leftIndex());
                Type rightType = graph->getOperandType(irGenericPlus->rightIndex());
                if (leftType.isInt32Type() && rightType.isInt32Type()) {
                    ESIR* int32PlusIR = Int32PlusIR::create(irGenericPlus->targetIndex(), irGenericPlus->leftIndex(), irGenericPlus->rightIndex());
                    block->replace(j, int32PlusIR);
                    graph->setOperandType(irGenericPlus->targetIndex(), TypeInt32);
                } else if (leftType.hasNumberFlag() && rightType.hasNumberFlag()) {
                    ESIR* doublePlusIR = DoublePlusIR::create(irGenericPlus->targetIndex(), irGenericPlus->leftIndex(), irGenericPlus->rightIndex());
                    block->replace(j, doublePlusIR);
                    graph->setOperandType(irGenericPlus->targetIndex(), TypeDouble);
                } else if (leftType.isStringType() || rightType.isStringType()) {
                    ESIR* stringPlusIR = StringPlusIR::create(irGenericPlus->targetIndex(), irGenericPlus->leftIndex(), irGenericPlus->rightIndex());
                    block->replace(j, stringPlusIR);
                    graph->setOperandType(irGenericPlus->targetIndex(), TypeString);
                } else {
                    printf("Unhandled GenericPlus case in ESGraphTypeInference\n");
                    RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            }
            case ESIR::Opcode::Minus:
                // FIXME
                graph->setOperandType(ir->targetIndex(), TypeInt32);
                break;
            case ESIR::Opcode::BitwiseAnd:
            case ESIR::Opcode::BitwiseOr:
            case ESIR::Opcode::BitwiseXor:
                graph->setOperandType(ir->targetIndex(), TypeInt32);
                break;
            case ESIR::Opcode::Equal:
            case ESIR::Opcode::NotEqual:
            case ESIR::Opcode::StrictEqual:
            case ESIR::Opcode::NotStrictEqual:
            case ESIR::Opcode::GreaterThan:
            case ESIR::Opcode::GreaterThanOrEqual:
            case ESIR::Opcode::LessThan:
            case ESIR::Opcode::LessThanOrEqual:
                graph->setOperandType(ir->targetIndex(), TypeBoolean);
                break;
            case ESIR::Opcode::LeftShift:
            case ESIR::Opcode::SignedRightShift:
            case ESIR::Opcode::UnsignedRightShift:
                graph->setOperandType(ir->targetIndex(), TypeInt32);
                break;
            case ESIR::Opcode::Jump:
            case ESIR::Opcode::Branch:
            case ESIR::Opcode::CallJS:
            case ESIR::Opcode::Return:
            case ESIR::Opcode::ReturnWithValue:
                break;
            case ESIR::Opcode::GetArgument:
            case ESIR::Opcode::GetVar:
            case ESIR::Opcode::GetVarGeneric:
            case ESIR::Opcode::GetObject:
                break;
            case ESIR::Opcode::SetVar:
            {
                SetVarIR* irSetVar = static_cast<SetVarIR*>(ir);
                Type setType = graph->getOperandType(irSetVar->sourceIndex());
                graph->setOperandType(ir->targetIndex(), setType);
                break;
            }
            case ESIR::Opcode::SetVarGeneric:
            {
                INIT_ESIR(SetVarGeneric);
                Type setType = graph->getOperandType(irSetVarGeneric->sourceIndex());
                graph->setOperandType(ir->targetIndex(), setType);
                break;
            }
            case ESIR::Opcode::ToNumber:
            case ESIR::Opcode::Increment:
            {
                ToNumberIR* irToNumber = static_cast<ToNumberIR*>(ir);
                Type srcType = graph->getOperandType(irToNumber->sourceIndex());
                if (srcType.isInt32Type()) {
                    graph->setOperandType(ir->targetIndex(), TypeInt32);
                } else {
                    graph->setOperandType(ir->targetIndex(), TypeDouble);
                  }
                break;
            }
            case ESIR::Opcode::PutInObject:
            {
                PutInObjectIR* irPutInObject = static_cast<PutInObjectIR*>(ir);
                Type srcType = graph->getOperandType(irPutInObject->sourceIndex());
                graph->setOperandType(ir->targetIndex(), srcType);
                break;
            }
            default:
                printf("ERROR %s not handled in ESGraphTypeInference.\n", ir->getOpcodeName());
                RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

#ifndef NDEBUG
    if (ESVMInstance::currentInstance()->m_verboseJIT)
        graph->dump(std::cout, "After running Type Inference");
#endif
}

}}
#endif
