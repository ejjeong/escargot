#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITMiddleend.h"

#include "ESGraph.h"
#include "ESIR.h"

namespace escargot {
namespace ESJIT {

bool ESGraphTypeModifier::run(ESGraph* graph)
{
    bool hasDouble = false;
    for (size_t i = 0; i < graph->operandsSize(); i++) {
        Type tp = graph->getOperandType(i);
        if(tp.isDoubleType()) {
            hasDouble = true;
            break;
        }
    }

    if(hasDouble) {
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
