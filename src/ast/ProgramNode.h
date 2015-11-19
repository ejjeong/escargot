#ifndef ProgramNode_h
#define ProgramNode_h

#include "Node.h"
#include "StatementNode.h"

namespace escargot {

class ProgramNode : public Node {
public:
    friend class ScriptParser;
    ProgramNode(StatementNodeVector&& body, bool isStrict)
        : Node(NodeType::Program)
    {
        m_body = body;
        m_isStrict = isStrict;
        m_roughCodeblockSizeInWordSize = 0;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for (unsigned i = 0; i < m_body.size() ; i ++) {
            m_body[i]->generateStatementByteCode(codeBlock, context);
#ifndef NDEBUG
            codeBlock->pushCode(CheckStackPointer(this->m_sourceLocation.m_lineNumber), context, this);
#endif
        }
        codeBlock->pushCode(End(), context, this);
        codeBlock->m_isStrict = m_isStrict;
    }

    StatementNodeVector body()
    {
        return m_body;
    }

    size_t roughCodeblockSizeInWordSize()
    {
        return m_roughCodeblockSizeInWordSize;
    }

    void setRoughCodeblockSizeInWordSize(size_t siz)
    {
        m_roughCodeblockSizeInWordSize = siz;
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        for (unsigned i = 0; i < m_body.size() ; i ++) {
            m_body[i]->computeRoughCodeBlockSizeInWordSize(m_roughCodeblockSizeInWordSize);
        }
    }

protected:
    StatementNodeVector m_body; // body: [ Statement ];
    bool m_isStrict;
    size_t m_roughCodeblockSizeInWordSize;
};

}

#endif
