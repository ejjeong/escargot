#ifndef BinaryExpressionNode_h
#define BinaryExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionNode : public ExpressionNode {
public:
    enum BinaryExpressionOperator {
        PLUS,  //"+"
        MINUS,  //"-"
        EQUALTO, //"=="
        LESSTHAN, //"<"
        GREATERTHAN, //">"
        BITWISEAND, //"&"
    };

    BinaryExpressionNode(Node *left, Node* right, const ESString& oper)
            : ExpressionNode(NodeType::BinaryExpression)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;

        if (oper == L"+")
            m_operator = PLUS;
        else if (oper == L"-")
            m_operator = MINUS;
        else if (oper == L"<")
            m_operator = LESSTHAN;
        else if (oper == L"&")
            m_operator = BITWISEAND;
        else
            RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESValue* execute(ESVMInstance* instance)
    {
        ESValue *lval = m_left->execute(instance)->ensureValue();
        ESValue *rval = m_right->execute(instance)->ensureValue();
        ESValue *ret;
        switch(m_operator) {
            case PLUS:
                /* http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1 */
                lval = lval->toPrimitive();
                rval = rval->toPrimitive();
                if ((lval->isHeapObject() && lval->toHeapObject()->isString())
                    || (rval->isHeapObject() && rval->toHeapObject()->isString())) {
                    // TODO
                } else {
                    if (lval->isSmi() && rval->isSmi())
                        ret = Smi::fromInt(lval->toSmi()->value() + rval->toSmi()->value());
                    else {
                        double lnum = lval->isSmi()? lval->toSmi()->value() : lval->toHeapObject()->toNumber()->get();
                        double rnum = rval->isSmi()? rval->toSmi()->value() : rval->toHeapObject()->toNumber()->get();
                        ret = Number::create(lnum + rnum);
                    }
                }
                break;
            case LESSTHAN:
                /* http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.1
                 * http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5 */
                lval = lval->toPrimitive();
                rval = rval->toPrimitive();

                // TODO http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
                // string, NaN, zero, infinity, ...
                if (lval->isSmi() && rval->isSmi())
                    ret = Boolean::create(lval->toSmi()->value() < rval->toSmi()->value());
                else {
                    double lnum = lval->isSmi()? lval->toSmi()->value() : lval->toHeapObject()->toNumber()->get();
                    double rnum = rval->isSmi()? rval->toSmi()->value() : rval->toHeapObject()->toNumber()->get();
                    ret = Boolean::create(lnum < rnum);
                }
                break;
            default:
                // TODO
                RELEASE_ASSERT_NOT_REACHED();
                break;
        }
        return ret;
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
    BinaryExpressionOperator m_operator;
};

}

#endif
