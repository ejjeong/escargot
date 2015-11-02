#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITMiddleend.h"

#include "ESGraph.h"
#include "ESIR.h"

namespace escargot {
namespace ESJIT {

void computeDominanceFrontier(ESGraph* graph)
{
}

bool ESGraphSSAConversion::run(ESGraph* graph)
{
    computeDominanceFrontier(graph);
    for (size_t i = 0; i < graph->basicBlockSize(); i++) {
        ESBasicBlock* block = graph->basicBlock(i);
        for (size_t j = 0; j < block->instructionSize(); j++) {
            ESIR* ir = block->instruction(j);
#if 0
            switch (ir->opcode()) {
            case SetVar:
                block->dominanceFrontier()->addPhi(ir);
            }
#endif
        }
    }
#ifndef NDEBUG
    // if (ESVMInstance::currentInstance()->m_verboseJIT)
    // graph->dump(std::cout, "After running SSA conversion");
#endif
    return true;
}

}}
#endif



