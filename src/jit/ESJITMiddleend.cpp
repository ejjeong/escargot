#include "Escargot.h"
#include "ESJITMiddleend.h"

#include "ESIR.h"
#include "ESJIT.h"

namespace escargot {
namespace ESJIT {

void optimizeIR(ESGraph* graph)
{
#if 0
    IRGraphSimplification::run(graph);
    IRGraphLoadElimiation::run(graph);
    IRGraphTypeCheckHoisting::run(graph);
    IRGraphLoopInvariantCodeMotion::run(graph);
    IRGraphDeadCodeEliminiation::run(graph);
    IRGraphCommonSubexpressionElimination::run(graph);
    IRGraphGlobalValueNumbering::run(graph);
#endif

    return;
}

}}
