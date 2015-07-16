#ifndef BlockStatementNode_h
#define BlockStatementNode_h

#include "StatementNode.h"

namespace escargot {

//A block statement, i.e., a sequence of statements surrounded by braces.
class BlockStatementNode : public StatementNode {
public:
    BlockStatementNode(StatementNodeVector&& body)
            : StatementNode(NodeType::BlockStatement)
    {
        m_body = body;
    }

    virtual ESValue* execute(ESVMInstance* instance)
    {
        for(unsigned i = 0; i < m_body.size() ; i ++) {
            m_body[i]->execute(instance);
        }
        return undefined;
    }

protected:
    StatementNodeVector m_body;// body: [ Statement ];
};

}

#endif
