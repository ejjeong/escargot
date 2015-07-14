#ifndef VariableDeclaratorNode_h
#define VariableDeclaratorNode_h

#include "Node.h"
#include "PatternNode.h"
#include "ExpressionNode.h"

namespace escargot {

class VariableDeclaratorNode : public Node {
public:
    VariableDeclaratorNode()
            : Node(NodeType::VariableDeclarator)
    {
        m_id = NULL;
        m_init = NULL;
    }
    virtual void execute(ESVMInstance* ) { }
protected:
    PatternNode* m_id; //id: Pattern;
    ExpressionNode* m_init; //init: Expression | null;
};

}

#endif
