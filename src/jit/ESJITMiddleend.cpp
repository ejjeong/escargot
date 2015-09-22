#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITMiddleend.h"

#include "ESIR.h"
#include "ESJIT.h"

namespace escargot {
namespace ESJIT {

void optimizeIR(ESGraph* graph)
{
    ESGraphSSAConversion::run(graph);
    ESGraphTypeInference::run(graph);
#if 0
    ESGraphSimplification::run(graph);
    ESGraphLoadElimiation::run(graph);
    ESGraphTypeCheckHoisting::run(graph);
    ESGraphLoopInvariantCodeMotion::run(graph);
    ESGraphDeadCodeEliminiation::run(graph);
    ESGraphCommonSubexpressionElimination::run(graph);
    ESGraphGlobalValueNumbering::run(graph);
#endif

    return;
}

}}
#endif
