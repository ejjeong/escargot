#ifndef ESIROperand_h
#define ESIROperand_h

#ifdef ENABLE_ESJIT

#include "ESIRType.h"

namespace escargot {
namespace ESJIT {

class ESIROperand {
public:
    void setType(Type& type) { m_type = type; }
    void mergeType(Type& type) { m_type.mergeType(type); }
    Type getType() { return m_type; }
    void setStackPos(unsigned stackPos) { m_stackPos = stackPos; }
    unsigned getStackPos() { return m_stackPos; }
    void increaseFollowingPopCount() { m_followingPopCount++; }
    unsigned getFollowingPopCount() { return m_followingPopCount; }
    void setUsed() { m_used = true; }
    bool used() { return m_used; }

#ifndef NDEBUG
    friend std::ostream& operator<< (std::ostream& os, const ESIROperand& op);
    void dump(std::ostream& out, size_t index)
    {
        out << "tmp" << index << " ";
        m_type.dump(out);
        out << " (used:" << m_used << ")";
    }
#endif

private:
    Type m_type;
    unsigned m_stackPos;
    unsigned m_followingPopCount;
    bool m_used;
};

}}
#endif
#endif
