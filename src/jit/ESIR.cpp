#include "Escargot.h"
#include "ESIR.h"

namespace escargot {
namespace ESJIT {

#ifndef NDEBUG
const char* getOpcodeName(ESIR::Opcode opcode)
{
    switch (opcode) {
        #define DECLARE_OPCODE_NAME(name) case ESIR::name: return #name;
        FOR_EACH_ESIR_OP(DECLARE_OPCODE_NAME)
        #undef  DECLARE_OPCODE_NAME
        default: RELEASE_ASSERT_NOT_REACHED();
    }
}

void ESIR::dump(std::ostream& out)
{
    out << getOpcodeName(m_opcode);
}
#endif

}}
