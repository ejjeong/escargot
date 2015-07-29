#ifndef ThrowStatementNode_h
#define ThrowStatementNode_h

//#include "Node.h"
//#include "ExpressionNode.h"
//#include "IdentifierNode.h"
//#include "BlockStatementNode.h"

namespace escargot {

//interface ThrowStatement <: Statement {
class ThrowStatementNode : public Node {
public:
    ThrowStatementNode(Node *argument)
            : Node(NodeType::ThrowStatement)
    {
        m_argument = (ExpressionNode*) argument;
    }

    ESValue* execute(ESVMInstance* instance)
    {
       ESValue* arg = m_argument->execute(instance);
       if (arg->isHeapObject() && arg->toHeapObject()->isJSString()) {
           ESString str = arg->toESString();
           int jmp = 1;
        }
       wprintf(L"%ls\n", arg->toESString().data());
       return esUndefined;
    }

protected:
    ExpressionNode* m_argument;
};

}

#endif
