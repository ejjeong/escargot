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
        m_bodySize = m_body.size();
        m_rootedBody = m_body.data();
    }

    void execute(ESVMInstance* instance)
    {
        for(unsigned i = 0; i < m_bodySize ; i ++) {
            m_rootedBody[i]->executeStatement(instance);
        }
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for(unsigned i = 0; i < m_bodySize ; i ++) {
            m_rootedBody[i]->generateStatementByteCode(codeBlock, context);
        }
        codeBlock->pushCode(End(), this);
    }
protected:
    StatementNodeVector m_body; //body: [ Statement ];
    Node** m_rootedBody;
    size_t m_bodySize;
};

}

#endif
