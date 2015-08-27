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
            arr->enumeration([&propertyVals](const ESValue& key, const ::escargot::ESSlotAccessor& val) {
                propertyVals.push_back(key);
            });
        }
        ESObject* obj = exprValue.toObject();
        std::vector<ESString*> propertyNames;
        obj->enumeration([&propertyNames](ESString* key, const ::escargot::ESSlotAccessor& slot) {
            propertyNames.push_back(key);
        });
        ec->setJumpPositionAndExecute([&](){
            jmpbuf_wrapper cont;
            int r = setjmp(cont.m_buffer);
            if (r != 1) {
                ec->pushContinuePosition(cont);
            }
            for (unsigned int i=0; i<propertyVals.size(); i++) {
                ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);
                ESSlotAccessor slot = m_left->executeForWrite(instance);
                ESSlotWriterForAST::setValue(slot, ec, propertyVals[i]);
                m_body->execute(instance);
            }
            for (unsigned int i=0; i<propertyNames.size(); i++) {
                if (obj->hasOwnProperty(propertyNames[i])) {
                    ESString* name = propertyNames[i];
                    ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);
                    ESSlotAccessor slot = m_left->executeForWrite(instance);
                    ESSlotWriterForAST::setValue(slot, ec, name);
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
