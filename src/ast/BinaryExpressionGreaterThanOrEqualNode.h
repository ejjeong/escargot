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

    ESValue execute(ESVMInstance* instance)
    {
        ESValue lval = m_left->execute(instance);
        ESValue rval = m_right->execute(instance);
        /* http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.1
         * http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5 */
        lval = lval.toPrimitive();
        rval = rval.toPrimitive();

        // TODO http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
        // string, NaN, zero, infinity, ...
        bool b;
        if (lval.isESString() || rval.isESString()) {
            ESString* lstr;
            ESString* rstr;

            lstr = lval.toString();
            rstr = rval.toString();
            b = lstr->string() >= rstr->string();

        } else {
            if(lval.isInt32() && rval.isInt32()) {
                int lnum = lval.asInt32();
                int rnum = rval.asInt32();
                b = lnum >= rnum;
            } else {
                double lnum = lval.toNumber();
                double rnum = rval.toNumber();
                b = lnum >= rnum;
            }
        }

        return ESValue(b);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
