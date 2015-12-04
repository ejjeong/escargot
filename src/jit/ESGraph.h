#ifndef ESGraph_h
#define ESGraph_h

#ifdef ENABLE_ESJIT

#include "ESJIT.h"
#include "ESIROperand.h"

namespace nanojit {

class LIns;

}

namespace escargot {

class FunctionNode;

namespace ESJIT {

class ESIR;
class ESGraph;
class ESBasicBlock;

typedef std::vector<ESIR*, CustomAllocator<ESIR*> > ESIRVector;
typedef std::vector<ESBasicBlock*, CustomAllocator<ESBasicBlock*> > ESBasicBlockVector;

class ESBasicBlock : public ESJITAlloc {
    friend class NativeGenerator;
public:
    static ESBasicBlock* create(ESGraph* graph, ESBasicBlock* parentBlock = nullptr, bool setIndexLater = false)
    {
        ESBasicBlock* newBlock = new ESBasicBlock(graph, parentBlock, setIndexLater);
        return newBlock;
    }

    void push(ESIR* ir) { m_instructions.push_back(ir); }
    void replace(size_t index, ESIR* ir)
    {
        ASSERT(index <= instructionSize());
        m_instructions[index] = ir;
    }

    size_t instructionSize() { return m_instructions.size(); }
    ESIR* instruction(size_t index) { return m_instructions[index]; }

    ESBasicBlockVector* parents() { return &m_parents; }
    ESBasicBlockVector* children() { return &m_children; }
    void addParent(ESBasicBlock* parent) { m_parents.push_back(parent); }
    void addChild(ESBasicBlock* child) { m_children.push_back(child); }

    size_t index() { return m_index; }
    void setIndexLater(size_t index) { m_index = index; }
    bool endsWithJumpOrBranch();

    void setLabel(nanojit::LIns* label) { m_label = label; }
    nanojit::LIns* getLabel() { return m_label; }

    void addJumpOrBranchSource(nanojit::LIns* source)
    {
        m_jumpOrBranchSources.push_back(source);
    }

    void addInsToExtendLife(nanojit::LIns* ins)
    {
        m_insToExtendLife.push_back(ins);
    }

    std::vector<nanojit::LIns*, CustomAllocator<nanojit::LIns*> >* getInsToExtendLife()
    {
        return &m_insToExtendLife;
    }

    ESGraph* graph()
    {
        return m_graph;
    }

#ifndef NDEBUG
    void dump(std::ostream& out);
#endif

private:
    ESBasicBlock(ESGraph* graph, ESBasicBlock* parentBlock, bool setIndexLater);

    ESGraph* m_graph;
    ESBasicBlockVector m_parents;
    ESBasicBlockVector m_children;
    ESIRVector m_instructions;
    size_t m_index;
    nanojit::LIns* m_label;
    std::vector<nanojit::LIns*, CustomAllocator<nanojit::LIns*> > m_jumpOrBranchSources;
    ESBasicBlock* m_dominanceFrontier;
    std::vector<nanojit::LIns*, CustomAllocator<nanojit::LIns*> > m_insToExtendLife;
};

class ESGraph : public ESJITAlloc {
    friend class NativeGenerator;
public:
    static ESGraph* create(CodeBlock* codeBlock)
    {
        ASSERT(codeBlock);
        return new ESGraph(codeBlock);
    }
    size_t basicBlockSize() { return m_basicBlocks.size(); }
    ESBasicBlock* basicBlock(size_t index) { return m_basicBlocks[index]; }
    void push(ESBasicBlock* bb) { m_basicBlocks.push_back(bb); }

    int tempRegisterSize();
    size_t operandsSize() { return m_operands.size(); }
    void setOperandType(int index, Type type)
    {
        ASSERT(index != -1);
        m_operands[index].setType(type);
    }
    void mergeOperandType(int index, Type type) { m_operands[index].mergeType(type); }
    Type getOperandType(int index) { return m_operands[index].getType(); }
    void setOperandStackPos(int index, unsigned stackPos)
    {
        ASSERT(index < (int)m_operands.size());
        m_operands[index].setStackPos(stackPos);
        m_lastStackPosSettingTargetIndex = index;
    }
    unsigned getOperandStackPos(int index) { return m_operands[index].getStackPos(); }
    unsigned lastStackPosSettingTargetIndex() { return m_lastStackPosSettingTargetIndex; }
    void increaseFollowingPopCountOf(int index) { m_operands[index].increaseFollowingPopCount(); }
    unsigned getFollowPopCountOf(int index) { return m_operands[index].getFollowingPopCount(); }

    void setOperandUsed(int index) { m_operands[index].setUsed(); }
    bool getOperandUsed(int index) { return m_operands[index].used(); }

    CodeBlock* codeBlock() { return m_codeBlock; }

#ifndef NDEBUG
    void dump(std::ostream& out, const char* msg = nullptr);
#endif

private:
    ESGraph(CodeBlock* codeBlock);

    ESBasicBlockVector m_basicBlocks;
    CodeBlock* m_codeBlock;
    std::vector<ESIROperand, CustomAllocator<ESIROperand> > m_operands;
    unsigned m_lastStackPosSettingTargetIndex;
};

}}
#endif
#endif
