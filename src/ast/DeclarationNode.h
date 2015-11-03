#ifndef DeclarationNode_h
#define DeclarationNode_h

#include "StatementNode.h"

namespace escargot {

// Any declaration node. Note that declarations are considered statements; this is because declarations can appear in any statement context in the language recognized by the SpiderMonkey parser.
class DeclarationNode : public StatementNode {
public:
    DeclarationNode(NodeType type)
        : StatementNode(type)
    {
    }
protected:
};

}


#endif
