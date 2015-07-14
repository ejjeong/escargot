#ifndef VariableDeclaratorNode_h
#define VariableDeclaratorNode_h

#include "Node.h"
#include "PatternNode.h"
#include "ExpressionNode.h"

namespace escargot {

class VariableDeclaratorNode : public Node {
public:
    VariableDeclaratorNode(Node* id)
            : Node(NodeType::VariableDeclarator)
    {
        m_id = id;
        m_init = NULL;
    }
    virtual void execute(ESVMInstance* ) { }
protected:
    Node* m_id; //id: Pattern;
    ExpressionNode* m_init; //init: Expression | null;
};


typedef std::vector<Node *, gc_allocator<Node *>> VariableDeclaratorVector;

}

#endif
