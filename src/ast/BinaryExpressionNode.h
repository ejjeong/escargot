#ifndef BinaryExpressionNode_h
#define BinaryExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionNode : public ExpressionNode {
public:
    enum BinaryExpressionOperator {
        // TODO

        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6
        // Additive Operators
        Plus,  //"+"
        Minus,  //"-"

        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.7
        // Bitwise Shift Operators
        LeftShift, //"<<"
        SignedRightShift, //">>"
        UnsignedRightShift, //">>>"

        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.8
        // Relational Operators
        Lessthan, //"<"
        GreaterThan, //">"
        // TODO

        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.9
        // Equality operators
        Equals, //"=="
        NotEquals, //"!="
        // TODO

        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.10
        // Binary Bitwise operators
        BitwiseAnd, //"&"
        // TODO
    };

    BinaryExpressionNode(Node *left, Node* right, const ESString& oper)
            : ExpressionNode(NodeType::BinaryExpression)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;

        // Additive Operators
        if (oper == L"+")
            m_operator = Plus;
        else if (oper == L"-")
            m_operator = Minus;

        // Relational Operators
        else if (oper == L"<")
            m_operator = Lessthan;

        // Equality Operators
        else if (oper == L"!=")
            m_operator = NotEquals;

        // Binary Bitwise Operator
        else if (oper == L"&")
            m_operator = BitwiseAnd;

        // TODO
        else
            RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESValue* execute(ESVMInstance* instance)
    {
        ESValue* lval = m_left->execute(instance)->ensureValue();
        ESValue* rval = m_right->execute(instance)->ensureValue();
        return execute(instance, lval, rval, m_operator);
    }
    static ESValue* execute(ESVMInstance* instance, ESValue* lval, ESValue* rval, BinaryExpressionOperator oper) {
        ESValue* ret;
        switch(oper) {
            case Plus:
                /* http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1 */
                lval = lval->toPrimitive();
                rval = rval->toPrimitive();
                if ((lval->isHeapObject() && lval->toHeapObject()->isJSString())
                    || (rval->isHeapObject() && rval->toHeapObject()->isJSString())) {
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
            case Lessthan:
                /* http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.1
                 * http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5 */
                lval = lval->toPrimitive();
                rval = rval->toPrimitive();

                // TODO http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
                // string, NaN, zero, infinity, ...
                if (lval->isSmi() && rval->isSmi()) {
                    bool b = lval->toSmi()->value() < rval->toSmi()->value();
                    if(b)
                        ret = esTrue;
                    else
                        ret = esFalse;
                }
                else {
                    double lnum = lval->isSmi()? lval->toSmi()->value() : lval->toHeapObject()->toNumber()->get();
                    double rnum = rval->isSmi()? rval->toSmi()->value() : rval->toHeapObject()->toNumber()->get();
                    bool b = lnum < rnum;
                    if(b)
                        ret = esTrue;
                    else
                        ret = esFalse;
                }
                break;
            case BitwiseAnd:
                lval = lval->toInt32();
                rval = rval->toInt32();

                /* http://www.ecma-international.org/ecma-262/5.1/#sec-11.10 */
                if (lval->isSmi() && rval->isSmi()) {
                    ret = Smi::fromInt(lval->toSmi()->value() & rval->toSmi()->value());
                } else {
                    // TODO
                }
                break;
            case LeftShift:
            {
                lval = lval->toInt32();
                rval = rval->toInt32();
                long long int rnum = rval->isSmi()? rval->toSmi()->value() : rval->toHeapObject()->toNumber()->get();
                long long int lnum = lval->isSmi()? lval->toSmi()->value() : lval->toHeapObject()->toNumber()->get();
                int shiftCount = rnum & 0x1F;
                lnum <<= shiftCount;
                if (lnum >= 40000000)
                    ret = Number::create(lnum);
                else
                    ret = Smi::fromInt(lnum);
                break;
            }
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
