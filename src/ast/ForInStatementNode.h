#ifndef ForInStatementNode_h
#define ForInStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"
#include "AssignmentExpressionNode.h"

namespace escargot {

class ForInStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    ForInStatementNode(Node *left, Node *right, Node *body, bool each)
            : StatementNode(NodeType::ForInStatement)
    {
        m_left = (ExpressionNode*) left;
        m_right = (ExpressionNode*) right;
        m_body = (StatementNode*) body;
        m_each = each;
    }

    virtual ESValue execute(ESVMInstance* instance)
    {
        ESValue exprValue = m_right->execute(instance);
        if (exprValue.isNull() || exprValue.isUndefined())
            return ESValue();

        ESObject* obj = exprValue.toObject();
        std::vector<InternalString> propertyNames;
        obj->enumeration([&propertyNames](const InternalString& key, ESSlot* slot) {
            propertyNames.push_back(key);
        });
        instance->currentExecutionContext()->setJumpPositionAndExecute([&](){
            jmpbuf_wrapper cont;
            int r = setjmp(cont.m_buffer);
            if (r != 1) {
                instance->currentExecutionContext()->pushContinuePosition(cont);
            }
            for (unsigned int i=0; i<propertyNames.size(); i++) {
                if (obj->hasKey(propertyNames[i])) {
                    ESString* name = ESString::create(propertyNames[i]);
                    AssignmentExpressionNode::writeValue(instance, m_left, name);
                    m_body->execute(instance);
                }
            }
            instance->currentExecutionContext()->popContinuePosition();
        });
        return ESValue();
    }

protected:
    ExpressionNode *m_left;
    ExpressionNode *m_right;
    StatementNode *m_body;
    bool m_each;
};

}

#endif
