#ifndef ProgramNode_h
#define ProgramNode_h

#include "Node.h"
#include "StatementNode.h"

namespace escargot {

class ProgramNode : public Node {
public:
    friend class ESScriptParser;
    ProgramNode(StatementNodeVector&& body)
            : Node(NodeType::Program)
    {
        m_body = body;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for(unsigned i = 0; i < m_body.size() ; i ++) {
            m_body[i]->generateStatementByteCode(codeBlock, context);
#ifndef NDEBUG
        codeBlock->pushCode(CheckStackPointer(this->m_sourceLocation.m_lineNumber), this);
#endif
        }
        codeBlock->pushCode(End(), this);
    }
protected:
    StatementNodeVector m_body; //body: [ Statement ];
};

}

#endif
