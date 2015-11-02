#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITMiddleend.h"

#include "ESIR.h"
#include "ESJIT.h"

namespace escargot {
namespace ESJIT {

bool optimizeIR(ESGraph* graph)
{
    if (!ESGraphSSAConversion::run(graph)) {
        LOG_VJ("Failed to run SSAConversion\n");
        return false;
    }

    if (!ESGraphTypeModifier::run(graph)) {
        LOG_VJ("Failed to run GraphTypeModifier\n");
        return false;
    }

    if (!ESGraphTypeInference::run(graph)) {
        LOG_VJ("Failed to run TypeInference\n");
        return false;
    }
#if 0
    ESGraphSimplification::run(graph);
    ESGraphLoadElimiation::run(graph);
    ESGraphTypeCheckHoisting::run(graph);
    ESGraphLoopInvariantCodeMotion::run(graph);
    ESGraphDeadCodeEliminiation::run(graph);
    ESGraphCommonSubexpressionElimination::run(graph);
    ESGraphGlobalValueNumbering::run(graph);
#endif

    return true;
}

}}
#endif



