#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESGraph.h"

#include "ESIR.h"
#include "bytecode/ByteCode.h"

#include "nanojit.h"

namespace escargot {
namespace ESJIT {

ESBasicBlock::ESBasicBlock(ESGraph* graph, ESBasicBlock* parentBlock, bool setIndexLater)
    : m_graph(graph)
    , m_label(nullptr)
{
    if (setIndexLater) {
        m_index = SIZE_MAX;
    } else {
        m_index = 0;
        int tmp = graph->basicBlockSize();
        for (int i = graph->basicBlockSize() - 1; i >= 0; i--) {
            int blockIndex = graph->basicBlock(i)->index();
            if (blockIndex >= 0) {
                m_index = blockIndex + 1;
                break;
            }
        }
        graph->push(this);
    }

    if (parentBlock) {
        this->addParent(parentBlock);
        parentBlock->addChild(this);
    }
}

bool ESBasicBlock::endsWithJumpOrBranch()
{
    if (m_instructions.empty())
        return false;
    ESIR* ir = m_instructions.back();
    switch (ir->opcode()) {
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
#endif

ESGraph::ESGraph(CodeBlock* codeBlock)
    : m_codeBlock(codeBlock),
    m_operands(m_codeBlock->m_tempRegisterSize)
{
}

int ESGraph::tempRegisterSize()
{
    return m_codeBlock->m_tempRegisterSize;
}

#ifndef NDEBUG
void ESGraph::dump(std::ostream& out, const char* msg)
{
    out << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    out << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    out << " ESGraph 0x" << std::hex << reinterpret_cast<uint64_t>(this) << std::dec ;
    out << " (" << m_basicBlocks.size() << " basic blocks) : " << (msg?msg:"") << std::endl;
    // out << " name : " << /*m_codeBlock->m_nonAtomicId->utf8Data()*/ ", address "  << std::hex << (void*)(this) << std::dec << std::endl;
    for (size_t i = 0; i < m_basicBlocks.size(); i++) {
        m_basicBlocks[i]->dump(out);
    }
    out << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    for (size_t i = 0; i < m_operands.size(); i++) {
        m_operands[i].dump(out, i);
        out << std::endl;
    }
    out << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    out << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
}

#endif

}}
#endif
