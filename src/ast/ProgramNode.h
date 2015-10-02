#ifndef ProgramNode_h
#define ProgramNode_h

#include "Node.h"
#include "StatementNode.h"

namespace escargot {

class ProgramNode : public Node {
public:
    friend class ESScriptParser;
    ProgramNode(StatementNodeVector&& body, bool isStrict)
            : Node(NodeType::Program)
    {
        m_body = body;
        m_isStrict = isStrict;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
#ifdef ENABLE_ESJIT
        context.setCurrentNodeIndex(0);
#endif
        for(unsigned i = 0; i < m_body.size() ; i ++) {
            m_body[i]->generateStatementByteCode(codeBlock, context);
#ifndef NDEBUG
        codeBlock->pushCode(CheckStackPointer(this->m_sourceLocation.m_lineNumber), this);
#endif
        }
        codeBlock->pushCode(End(), this);
        codeBlock->m_isStrict = m_isStrict;
    }
protected:
    StatementNodeVector m_body; //body: [ Statement ];
    bool m_isStrict;
};

}

#endif
