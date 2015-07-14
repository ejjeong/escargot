#ifndef ProgramNode_h
#define ProgramNode_h

#include "Node.h"
#include "StatementNode.h"

namespace escargot {

class ProgramNode : public Node {
public:
    ProgramNode(StatementNodeVector&& body)
            : Node(NodeType::Program)
    {
        m_body = body;
    }
    virtual void execute(ESVMInstance* ) { }
protected:
    StatementNodeVector m_body; //body: [ Statement ];
};

}

#endif
