#include "Escargot.h"
#include "ESIR.h"

#include "ESGraph.h"

namespace escargot {
namespace ESJIT {

const char* ESIR::getOpcodeName()
{
    switch (m_opcode) {
        #define DECLARE_OPCODE_NAME(name) case ESIR::name: return #name;
        FOR_EACH_ESIR_OP(DECLARE_OPCODE_NAME)
        #undef  DECLARE_OPCODE_NAME
        default: RELEASE_ASSERT_NOT_REACHED();
    }
}

#ifndef NDEBUG
void ESIR::dump(std::ostream& out)
{
    out << getOpcodeName();
}

void BranchIR::dump(std::ostream& out)
{
    out << "tmp" << m_targetIndex << ": ";
    ESIR::dump(out);
    out << " if tmp" << m_operandIndex;
    out << ", goto B" << m_trueBlock->index() << ", else goto B" << m_falseBlock->index();
}

void JumpIR::dump(std::ostream& out)
{
    out << "tmp" << m_targetIndex << ": ";
    ESIR::dump(out);
    out << " goto B" << m_targetBlock->index();
}
#endif

}}
