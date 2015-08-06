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
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue ret;
        size_t siz = m_body.size();
        for(unsigned i = 0; i < siz ; ++ i) {
            ESValue s = m_body[i]->execute(instance);
            if (!s.isEmpty())
                ret = s;
        }
        return ret;
    }

protected:
    StatementNodeVector m_body;// body: [ Statement ];
};

}

#endif
