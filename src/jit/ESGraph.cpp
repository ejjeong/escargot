#include "Escargot.h"
#include "ESGraph.h"

#include "ESIR.h"
#include "bytecode/ByteCode.h"

namespace escargot {
namespace ESJIT {

ESBasicBlock::ESBasicBlock(ESGraph* graph, ESBasicBlock* parentBlock)
    : m_index(graph->basicBlockSize())
      , m_label(nullptr)
{
    graph->push(this);
    if (parentBlock) {
        this->addParent(parentBlock);
        parentBlock->addChild(this);
    }
}

bool ESBasicBlock::endsWithJumpOrBranch()
{
    ESIR* ir = m_instructions.back();
    switch(ir->opcode()) {
        case ESIR::Opcode::Jump:
        case ESIR::Opcode::Branch:
            return true;
        default:
            return false;
    }
}

#ifndef NDEBUG
void ESBasicBlock::dump(std::ostream& out)
{
    out << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    out << "Block #" << m_index;
    out << " (" << m_instructions.size() << " Instructions)\n";
    out << "Parents: ";
    for (size_t i = 0; i < m_parents.size(); i++)
        out << "Block #" << m_parents[i]->m_index << ", ";
    out << std::endl;
    out << "Children: ";
    for (size_t i = 0; i < m_children.size(); i++)
        out << "Block #" << m_children[i]->m_index << ", ";
    out << std::endl;

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
        m_basicBlocks[i]->dump(out);
    }
    out << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
}
#endif

int ESGraph::tempRegisterSize()
{
    return m_codeBlock->m_tempRegisterSize;
}

}}
