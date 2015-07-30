#ifndef ReturnStatmentNode_h
#define ReturnStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ReturnStatmentNode : public StatementNode {
public:
    friend class ESScriptParser;
    ReturnStatmentNode(Node* argument)
            : StatementNode(NodeType::ReturnStatement)
    {
        m_argument = argument;
    }

    virtual ESValue* execute(ESVMInstance* instance);
protected:
    Node* m_argument;
};

}

#endif
