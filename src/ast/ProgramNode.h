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

    ESValue execute(ESVMInstance* instance)
    {
        ESValue ret;
        size_t siz = m_body.size();
        for(unsigned i = 0; i < siz ; i ++) {
            ESValue s = m_body[i]->execute(instance);
            if (!s.isEmpty())
                ret = s;
        }
        return ret;
    }
protected:
    StatementNodeVector m_body; //body: [ Statement ];
};

}

#endif
