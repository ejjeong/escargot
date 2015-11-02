#ifndef BlockStatementNode_h
#define BlockStatementNode_h

#include "StatementNode.h"

namespace escargot {

// A block statement, i.e., a sequence of statements surrounded by braces.
class BlockStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    BlockStatementNode(StatementNodeVector&& body)
        : StatementNode(NodeType::BlockStatement)
    {
        m_body = body;
    }


    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for (unsigned i = 0; i < m_body.size() ; ++ i) {
            m_body[i]->generateStatementByteCode(codeBlock, context);
        }
    }

protected:
    StatementNodeVector m_body; // body: [ Statement ];
};


}

#endif



