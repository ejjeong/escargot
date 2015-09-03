#ifndef AssignmentExpressionSimpleLeftIdentifierFastCaseNode_h
#define AssignmentExpressionSimpleLeftIdentifierFastCaseNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionSimpleLeftIdentifierFastCaseNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    AssignmentExpressionSimpleLeftIdentifierFastCaseNode(size_t idx, Node* right)
            : ExpressionNode(NodeType::AssignmentExpressionSimpleLeftIdentifierFastCase)
    {
        m_index = idx;
        m_right = right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        m_rvalue = m_right->execute(instance);
        instance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[m_index] = m_rvalue;
        return m_rvalue;
    }

protected:
    size_t m_index;
    ESValue m_rvalue;
    Node* m_right; //right: Expression;
};

}

#endif
