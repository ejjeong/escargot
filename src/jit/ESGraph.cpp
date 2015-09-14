#include "Escargot.h"
#include "ESGraph.h"

#include "ESIR.h"

namespace escargot {
namespace ESJIT {

#ifndef NDEBUG
void ESBasicBlock::dump(std::ostream& out)
{
    for (size_t i = 0; i < m_instructions.size(); i++) {
        out << "[" << i << "] ";
        m_instructions[i]->dump(out);
        out << std::endl;
    }
}

void ESGraph::dump(std::ostream& out)
{
    out << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    out << "Graph (" << m_basicBlocks.size() << " basic blocks)\n";
    for (size_t i = 0; i < m_basicBlocks.size(); i++) {
        out << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        out << "Block #" << i;
        out << " (" << basicBlock(i)->instructionSize() << " Instructions)\n";
        m_basicBlocks[i]->dump(out);
    }
    out << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
}
#endif

}}
