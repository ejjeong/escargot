#ifndef ContinueLabelStatmentNode_h
#define ContinueLabelStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ContinueLabelStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    ContinueLabelStatementNode(size_t upIndex, ESString* label)
            : StatementNode(NodeType::ContinueLabelStatement)
    {
        m_upIndex = upIndex;
        m_label = label;
    }

protected:
    size_t m_upIndex;
    ESString* m_label; //for debug
};

}

#endif
