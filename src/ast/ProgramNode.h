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

    virtual ESValue execute(ESVMInstance* instance)
    {
        size_t siz = m_body.size();
        for(unsigned i = 0; i < siz ; i ++) {
            m_body[i]->execute(instance);
        }

        return ESValue();
    }
protected:
    StatementNodeVector m_body; //body: [ Statement ];
};

}

#endif
