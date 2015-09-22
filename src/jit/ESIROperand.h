#ifndef ESIROperand_h
#define ESIROperand_h

#ifdef ENABLE_ESJIT

#include "ESIRType.h"

class CodeBlock;

namespace escargot {
namespace ESJIT {

class ESIROperand : public gc {
public:
    void setType(Type& type) { m_type = type; }
    void mergeType(Type& type) { m_type.mergeType(type); }
    Type getType() { return m_type; }

#ifndef NDEBUG
    friend std::ostream& operator<< (std::ostream& os, const ESIROperand& op);
    void dump(std::ostream& out, size_t index)
    {
        out << "tmp" << index << " ";
        m_type.dump(out);
    }
#endif

private:
    CodeBlock* m_codeBlock;
    Type m_type;
};

}}
#endif
#endif
