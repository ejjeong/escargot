#ifndef ForInStatementNode_h
#define ForInStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class ForInStatementNode : public StatementNode , public ControlFlowNode {
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

    void executeStatement(ESVMInstance* instance)
    {
        if(m_isSlowCase) {
            ESValue exprValue = m_right->executeExpression(instance);
            if (exprValue.isNull() || exprValue.isUndefined())
                return ;

            ExecutionContext* ec = instance->currentExecutionContext();
            ec->resetLastESObjectMetInMemberExpressionNode();

            std::vector<ESValue> propertyVals;
            ESObject* obj = exprValue.toObject();
            propertyVals.reserve(obj->keyCount());
            obj->enumeration([&propertyVals](ESValue key, const ::escargot::ESSlotAccessor& slot) {
                propertyVals.push_back(key);
            });
            /*
            std::sort(propertyVals.begin(), propertyVals.end(), [](const ::escargot::ESValue& a, const ::escargot::ESValue& b) -> bool {
                ::escargot::ESString* vala = a.toString();
                ::escargot::ESString* valb = b.toString();
                return vala->string() < valb->string();
            });*/

            ec->setJumpPositionAndExecute([&](){
                jmpbuf_wrapper cont;
                int r = setjmp(cont.m_buffer);
                if (r != 1) {
                    ec->pushContinuePosition(cont);
                }
                for (unsigned int i=0; i<propertyVals.size(); i++) {
                    if (obj->hasOwnProperty(propertyVals[i])) {
                        ESValue name = propertyVals[i];
                        ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);
                        ESSlotAccessor slot = m_left->executeForWrite(instance);
                        ESSlotWriterForAST::setValue(slot, ec, name);
                        m_body->executeStatement(instance);
                    }
                }
                instance->currentExecutionContext()->popContinuePosition();
            });
        } else {
            ESValue exprValue = m_right->executeExpression(instance);
            if (exprValue.isNull() || exprValue.isUndefined())
                return ;

            ExecutionContext* ec = instance->currentExecutionContext();
            ec->resetLastESObjectMetInMemberExpressionNode();

            std::vector<ESValue> propertyVals;
            ESObject* obj = exprValue.toObject();
            propertyVals.reserve(obj->keyCount());
            obj->enumeration([&propertyVals](ESValue key, const ::escargot::ESSlotAccessor& slot) {
                propertyVals.push_back(key);
            });

            /*
            std::sort(propertyVals.begin(), propertyVals.end(), [](const ::escargot::ESValue& a, const ::escargot::ESValue& b) -> bool {
                ::escargot::ESString* vala = a.toString();
                ::escargot::ESString* valb = b.toString();
                return vala->string() < valb->string();
            });
            */

            for (unsigned int i=0; i<propertyVals.size(); i++) {
                if (obj->hasOwnProperty(propertyVals[i])) {
                    ESValue name = propertyVals[i];
                    ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);
                    ESSlotAccessor slot = m_left->executeForWrite(instance);
                    ESSlotWriterForAST::setValue(slot, ec, name);
                    m_body->executeStatement(instance);
                }
            }
        }

    }

protected:
    ExpressionNode *m_left;
    ExpressionNode *m_right;
    StatementNode *m_body;
    bool m_each;
};

}

#endif
