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

    ESValue execute(ESVMInstance* instance)
    {
        ESValue exprValue = m_right->execute(instance);
        if (exprValue.isNull() || exprValue.isUndefined())
            return ESValue();

        ExecutionContext* ec = instance->currentExecutionContext();
        ec->resetLastESObjectMetInMemberExpressionNode();

        std::vector<ESValue> propertyVals;
        if (exprValue.isESPointer() && exprValue.asESPointer()->isESArrayObject()) {
            ESArrayObject* arr = exprValue.asESPointer()->asESArrayObject();
            arr->enumeration([&propertyVals](const ESValue& key, ESSlot* slot) {
                propertyVals.push_back(key);
            });
        }
        ESObject* obj = exprValue.toObject();
        std::vector<InternalString> propertyNames;
        obj->enumeration([&propertyNames](const InternalString& key, ESSlot* slot) {
            propertyNames.push_back(key);
        });
        ec->setJumpPositionAndExecute([&](){
            jmpbuf_wrapper cont;
            int r = setjmp(cont.m_buffer);
            if (r != 1) {
                ec->pushContinuePosition(cont);
            }
            for (unsigned int i=0; i<propertyVals.size(); i++) {
                ec->resetLastESObjectMetInMemberExpressionNode();
                ESSlot* slot = m_left->executeForWrite(instance);
                slot->setValue(propertyVals[i], ec->lastESObjectMetInMemberExpressionNode());
                m_body->execute(instance);
            }
            for (unsigned int i=0; i<propertyNames.size(); i++) {
                if (obj->hasKey(propertyNames[i])) {
                    ESString* name = ESString::create(propertyNames[i]);
                    ec->resetLastESObjectMetInMemberExpressionNode();
                    ESSlot* slot = m_left->executeForWrite(instance);
                    slot->setValue(name, ec->lastESObjectMetInMemberExpressionNode());
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
