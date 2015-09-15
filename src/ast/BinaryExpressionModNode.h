#ifndef BinaryExpressionModNode_h
#define BinaryExpressionModNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionModNode : public ExpressionNode {
public:
    BinaryExpressionModNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionMod)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;

    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESValue lval = m_left->executeExpression(instance);
        ESValue rval = m_right->executeExpression(instance);
        ESValue ret(ESValue::ESForceUninitialized);
        if (lval.isInt32() && rval.isInt32()) {
            ret = ESValue(lval.asInt32() % rval.asInt32());
        } else {
            double lvalue = lval.toNumber();
            double rvalue = rval.toNumber();
            // http://www.ecma-international.org/ecma-262/5.1/#sec-11.5.3
            if (std::isnan(lvalue) || std::isnan(rvalue))
                ret = ESValue(std::numeric_limits<double>::quiet_NaN());
            else if (lvalue == std::numeric_limits<double>::infinity() || lvalue == -std::numeric_limits<double>::infinity() || rvalue == 0 || rvalue == -0.0) {
                ret = ESValue(std::numeric_limits<double>::quiet_NaN());
            } else {
                bool isNeg = lvalue < 0;
                bool lisZero = lvalue == 0 || lvalue == -0.0;
                bool risZero = rvalue == 0 || rvalue == -0.0;
                if (!lisZero && (rvalue == std::numeric_limits<double>::infinity() || rvalue == -std::numeric_limits<double>::infinity()))
                    ret = ESValue(lvalue);
                else if (lisZero && !risZero)
                    ret = ESValue(lvalue);
                else {
                    int d = lvalue / rvalue;
                    ret = ESValue(lvalue - (d * rvalue));
                }
            }
        }
        return ret;
    }

    virtual void generateByteCode(CodeBlock* codeBlock)
    {
        m_left->generateByteCode(codeBlock);
        m_right->generateByteCode(codeBlock);
        codeBlock->pushCode(Mod(), this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
