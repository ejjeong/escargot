#ifndef ESGraph_h
#define ESGraph_h

#include <vector>
#include <iostream>

namespace escargot {

class FunctionNode;

namespace ESJIT {

class ESIR;

class ESBasicBlock : public gc {
public:
    size_t instructionSize() { return m_instructions.size(); }
    ESIR* instruction(size_t index) { return m_instructions[index]; }

    void addParent(ESBasicBlock* parent) { m_parents.push_back(parent); }
    void addChild(ESBasicBlock* child) { m_parents.push_back(child); }

    void push(ESIR* ir) { m_instructions.push_back(ir); }

#ifndef NDEBUG
    void dump(std::ostream& out);
#endif

private:
    std::vector<ESBasicBlock*> m_parents;
    std::vector<ESBasicBlock*> m_children;
    std::vector<ESIR*> m_instructions;
};

class ESGraph : public gc {
public:
    size_t basicBlockSize() { return m_basicBlocks.size(); }
    ESBasicBlock* basicBlock(size_t index) { return m_basicBlocks[index]; }
    void push(ESBasicBlock* bb) { m_basicBlocks.push_back(bb); }

#ifndef NDEBUG
    void dump(std::ostream& out);
#endif

private:
    std::vector<ESBasicBlock*> m_basicBlocks;
};

}}
#endif
