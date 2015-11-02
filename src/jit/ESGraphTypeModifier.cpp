#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITMiddleend.h"

#include "ESGraph.h"
#include "ESIR.h"

namespace escargot {
namespace ESJIT {

bool ESGraphTypeModifier::run(ESGraph* graph)
{
    unsigned doubleCnt = 0;
    unsigned intCnt = 0;
    for (size_t i = 0; i < graph->operandsSize(); i++) {
        Type tp = graph->getOperandType(i);
        if(tp.isDoubleType()) {
            doubleCnt ++;
        } else if(tp.isInt32Type()) {
            intCnt++;
        }
    }

    //printf("%f\n",(float)doubleCnt / (float)(intCnt + doubleCnt));
    //if((float)doubleCnt / (float)(intCnt + doubleCnt) > 0.1f) {
    //if(doubleCnt) {
    if(0) {
        for (size_t i = 0; i < graph->operandsSize(); i++) {
            Type tp = graph->getOperandType(i);
            if(tp.isInt32Type()) {
                graph->setOperandType(i, Type(TypeDouble));
            }
        }
    }
    return true;
}

}}
#endif
