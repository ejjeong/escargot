#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESIR.h"

#include "ESGraph.h"

namespace escargot {
namespace ESJIT {

const char* ESIR::getOpcodeName()
{
    switch (m_opcode) {
#define RETURN_OPCODE_NAME(name, unused) case ESIR::name: return #name;
        FOR_EACH_ESIR_OP(RETURN_OPCODE_NAME)
#undef  RETURN_OPCODE_NAME
        default: RELEASE_ASSERT_NOT_REACHED();
    }
}

uint32_t ESIR::getFlags()
{
    auto getFlag = [] (uint32_t flag = 0) -> uint32_t { return flag; };
    switch (m_opcode) {
#define RETURN_ESIR_FLAG(name, flag) case ESIR::name: return getFlag(flag);
        FOR_EACH_ESIR_OP(RETURN_ESIR_FLAG)
#undef  RETURN_ESIR_FLAG
        default: RELEASE_ASSERT_NOT_REACHED();
    }
}

bool ESIR::returnsESValue()
{
    return getFlags() & ReturnsESValue;
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
#endif

