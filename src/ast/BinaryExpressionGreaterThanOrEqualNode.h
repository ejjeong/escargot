#ifndef BinaryExpressionGreaterThanOrEqualNode_h
#define BinaryExpressionGreaterThanOrEqualNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionGreaterThanOrEqualNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionGreaterThanOrEqualNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionGreaterThanOrEqual)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        /* http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.1
         * http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5 */
        ESValue lval = m_left->executeExpression(instance).toPrimitive();
        ESValue rval = m_right->executeExpression(instance).toPrimitive();

        // TODO http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
        // string, NaN, zero, infinity, ...
        bool b;
        if(lval.isInt32() && rval.isInt32()) {
            b = lval.asInt32() >= rval.asInt32();
        } else if (lval.isESString() || rval.isESString()) {
            b = lval.toString()->string() >= rval.toString()->string();
        } else {
            b = lval.toNumber() >= rval.toNumber();
        }

        return ESValue(b);
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock)
    {
        m_left->generateExpressionByteCode(codeBlock);
        m_right->generateExpressionByteCode(codeBlock);
        codeBlock->pushCode(GreaterThanOrEqual(), this);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
