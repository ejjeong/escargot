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
        for(unsigned i = 0; i < m_body.size() ; i ++) {
            m_body[i]->execute(instance);
        }

        return ESValue();
    }
protected:
    StatementNodeVector m_body; //body: [ Statement ];
};

}

#endif
