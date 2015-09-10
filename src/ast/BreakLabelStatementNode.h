#ifndef BreakLabelStatmentNode_h
#define BreakLabelStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class BreakLabelStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    BreakLabelStatementNode(size_t upIndex, ESString* label)
            : StatementNode(NodeType::BreakLabelStatement)
    {
        m_upIndex = upIndex;
        m_label = label;
    }

    void executeStatement(ESVMInstance* instance)
    {
        instance->currentExecutionContext()->doLabeledBreak(m_upIndex);
    }
protected:
    size_t m_upIndex;
    ESString* m_label; //for debug
};

}

#endif
