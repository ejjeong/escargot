#ifndef AssignmentExpressionPlusNode_h
#define AssignmentExpressionPlusNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionPlusNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    AssignmentExpressionPlusNode(Node* left, Node* right)
            : ExpressionNode(NodeType::AssignmentExpressionPlus)
    {
        m_left = left;
        m_right = right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESSlotAccessor slot;
        ExecutionContext* ec = instance->currentExecutionContext();
        ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);

        ESValue ret(ESValue::ESForceUninitialized);

        slot = m_left->executeForWrite(instance);
        ESValue lval = slot.value(ec->lastESObjectMetInMemberExpressionNode()).toPrimitive();
        ESValue rval = m_right->executeExpression(instance).toPrimitive();

        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1
        if(lval.isInt32() && rval.isInt32()) {
            int a = lval.asInt32(), b = rval.asInt32();
            if (UNLIKELY(a > 0 && b > std::numeric_limits<int32_t>::max() - a)) {
                //overflow
                ret = ESValue((double)lval.asInt32() + (double)rval.asInt32());
            } else if (UNLIKELY(a < 0 && b < std::numeric_limits<int32_t>::min() - a)) {
                //underflow
                ret = ESValue((double)lval.asInt32() + (double)rval.asInt32());
            } else {
                ret = ESValue(lval.asInt32() + rval.asInt32());
            }
        } else if (lval.isESString() || rval.isESString()) {
            ret = ESString::concatTwoStrings(lval.toString(), rval.toString());
        } else {
            ret = ESValue(lval.toNumber() + rval.toNumber());
        }

        ESSlotWriterForAST::setValue(slot, ec, ret);
        return ret;
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
