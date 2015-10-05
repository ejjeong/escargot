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
            case ESIR::Opcode::GenericPlus:
            {
                // FIXME
            }
            case ESIR::Opcode::BitwiseAnd:
            case ESIR::Opcode::BitwiseOr:
            case ESIR::Opcode::BitwiseXor:
                graph->setOperandType(ir->targetIndex(), TypeInt32);
                break;
            case ESIR::Opcode::LessThan:
                graph->setOperandType(ir->targetIndex(), TypeBoolean);
                break;
            case ESIR::Opcode::LeftShift:
            case ESIR::Opcode::SignedRightShift:
            case ESIR::Opcode::UnsignedRightShift:
                graph->setOperandType(ir->targetIndex(), TypeInt32);
                break;
            case ESIR::Opcode::Jump:
            case ESIR::Opcode::Branch:
            case ESIR::Opcode::Return:
            case ESIR::Opcode::ReturnWithValue:
                break;
            case ESIR::Opcode::GetArgument:
            case ESIR::Opcode::GetVar:
                break;
            case ESIR::Opcode::SetVar:
            {
                SetVarIR* irSetVar = static_cast<SetVarIR*>(ir);
                Type setType = graph->getOperandType(irSetVar->sourceIndex());
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
            default:
                printf("ERROR %s not handled in ESGraphTypeInference.\n", ir->getOpcodeName());
                RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

#ifndef NDEBUG
    graph->dump(std::cout, "After running Type Inference");
#endif
}

}}
#endif
