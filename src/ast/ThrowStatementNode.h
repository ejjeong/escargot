#ifndef ThrowStatementNode_h
#define ThrowStatementNode_h

#include "StatementNode.h"

namespace escargot {

//interface ThrowStatement <: Statement {
class ThrowStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    ThrowStatementNode(Node *argument)
            : StatementNode(NodeType::ThrowStatement)
    {
        m_argument = argument;
    }

    ESValue execute(ESVMInstance* instance);
protected:
    Node* m_argument;
};

}

#endif
