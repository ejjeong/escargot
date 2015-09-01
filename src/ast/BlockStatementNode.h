#ifndef BlockStatementNode_h
#define BlockStatementNode_h

#include "StatementNode.h"

namespace escargot {

//A block statement, i.e., a sequence of statements surrounded by braces.
class BlockStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    BlockStatementNode(StatementNodeVector&& body)
            : StatementNode(NodeType::BlockStatement)
    {
        m_body = body;
        m_bodySize = m_body.size();
        m_rootedBody = m_body.data();
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue ret;
        for(unsigned i = 0; i < m_bodySize ; ++ i) {
            ESValue s = m_rootedBody[i]->execute(instance);
            if (!s.isEmpty())
                ret = s;
        }
        return ret;
    }

protected:
    StatementNodeVector m_body;// body: [ Statement ];
    Node** m_rootedBody;
    size_t m_bodySize;
};

}

#endif
