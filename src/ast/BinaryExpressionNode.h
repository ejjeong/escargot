#ifndef BinaryExpressionNode_h
#define BinaryExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    enum BinaryExpressionOperator {
        // TODO
        //
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.5
        // Multiplicative Operators
        Mult,  //"*"
        Div,   //"/"
        Mod,   //"%"

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
        LessThan, //"<"
        GreaterThan, //">"
        LessThanOrEqual, //"<="
        GreaterThanOrEqual, //">="
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

    BinaryExpressionNode(Node *left, Node* right, const InternalString& oper)
            : ExpressionNode(NodeType::BinaryExpression)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;

        // Additive Operators
        if (oper == L"+")
            m_operator = Plus;
        else if (oper == L"-")
            m_operator = Minus;

        // Bitwise Shift Operators
        else if (oper == L"<<")
            m_operator = LeftShift;
        else if (oper == L">>")
            m_operator = SignedRightShift;
        else if (oper == L">>>")
            m_operator = UnsignedRightShift;

        // Multiplicative Operators
        else if (oper == L"*")
            m_operator = Mult;
        else if (oper == L"/")
            m_operator = Div;

        // Relational Operators
        else if (oper == L"<")
            m_operator = LessThan;
        else if (oper == L">")
            m_operator = GreaterThan;
        else if (oper == L"<=")
            m_operator = LessThanOrEqual;
        else if (oper == L">=")
            m_operator = GreaterThanOrEqual;

        // Equality Operators
        else if (oper == L"==")
            m_operator = Equals;
        else if (oper == L"!=")
            m_operator = NotEquals;

        // Binary Bitwise Operator
        else if (oper == L"&")
            m_operator = BitwiseAnd;

        // TODO
        else
            RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESValue execute(ESVMInstance* instance)
    {
        ESValue lval = m_left->execute(instance).ensureValue();
        ESValue rval = m_right->execute(instance).ensureValue();
        return execute(instance, lval, rval, m_operator);
    }

    static ESValue execute(ESVMInstance* instance, ESValue lval, ESValue rval, BinaryExpressionOperator oper) {
        ESValue ret;
        switch(oper) {
            case Plus:
                /*
                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1
                lval = lval->toPrimitive();
                rval = rval->toPrimitive();
                if ((lval->isHeapObject() && lval->toHeapObject()->isESString())
                    || (rval->isHeapObject() && rval->toHeapObject()->isESString())) {
                    InternalString lstr;
                    InternalString rstr;
                    if (lval->isHeapObject() && lval->toHeapObject()->isESString())
                        lstr = lval->toHeapObject()->toESString()->string();
                    else
                        lstr = lval->toString()->string();

                    if (rval->isHeapObject() && rval->toHeapObject()->isESString())
                        rstr = rval->toHeapObject()->toESString()->string();
                    else
                        rstr = rval->toString()->string();

                    ret = ESString::create((*lstr.string() + *rstr.string()).c_str());
                } else {
                    if (lval->isSmi() && rval->isSmi())
                        ret = Smi::fromInt(lval->toSmi()->value() + rval->toSmi()->value());
                    else {
                        double lnum = lval->isSmi()? lval->toSmi()->value() : lval->toHeapObject()->toESNumber()->get();
                        double rnum = rval->isSmi()? rval->toSmi()->value() : rval->toHeapObject()->toESNumber()->get();
                        ret = ESNumber::create(lnum + rnum);
                        }
                    }
                    */
                break;
             case Minus:
                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.2
                if (lval.isInt32() && rval.isInt32() && !((lval.asInt32() | rval.asInt32()) & 0xc0000000)) // no overflow
                    ret = ESValue(lval.asInt32() - rval.asInt32());
                else
                    ret = ESValue(lval.toNumber() - rval.toNumber());
                break;
            case Div: {
                          /*
                lval = lval->toNumber();
                rval = rval->toNumber();
                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.5.2
                bool islNeg = lval->isSmi()? lval->toSmi()->value() < 0 : lval->toHeapObject()->toESNumber()->isNegative();
                bool isrNeg = rval->isSmi()? rval->toSmi()->value() < 0 : rval->toHeapObject()->toESNumber()->isNegative();
                bool islZero = lval->isSmi()? lval->toSmi()->value() == 0 : lval->toHeapObject()->toESNumber()->isZero();
                bool isrZero = rval->isSmi()? rval->toSmi()->value() == 0 : rval->toHeapObject()->toESNumber()->isZero();
                bool isNeg = (islNeg != isrNeg);
                if (lval == esNaN || rval == esNaN) ret = esNaN;
                else if (lval == esInfinity || lval == esNegInfinity) {
                    if (rval == esInfinity || rval == esNegInfinity)
                        ret = esNaN;
                    else { // if rval is zero or nonzero finite value
                        if (isNeg) ret = esNegInfinity;
                        else       ret = esInfinity;
                    }
                } else if (rval == esInfinity || rval == esNegInfinity) {
                    if (isNeg) ret = esMinusZero;
                    else       ret = Smi::fromInt(0);
                } else if (islZero) {
                    if (isrZero) ret = esNaN;
                    else {
                        if (isNeg) ret = esMinusZero;
                        else       ret = Smi::fromInt(0);
                    }
                } else if (isrZero) {
                    if (isNeg) ret = esNegInfinity;
                    else       ret = esInfinity;
                } else {
                    double lnum = lval->isSmi()? lval->toSmi()->value() : lval->toHeapObject()->toESNumber()->get();
                    double rnum = rval->isSmi()? rval->toSmi()->value() : rval->toHeapObject()->toESNumber()->get();
                    double result = lnum / rnum;

                    if (result == std::numeric_limits<double>::infinity())
                        ret = esInfinity;
                    else if (result == -std::numeric_limits<double>::infinity())
                        ret = esNegInfinity;
                    else if (result == -0.0)
                        ret = esMinusZero;
                    else
                        ret = ESNumber::create(result);
                }
                      */
                }
                break;
            case LessThan:
                /*
            case LessThanOrEqual:
            case GreaterThan:
            case GreaterThanOrEqual:
            */
            {
                if (lval.isInt32() && rval.isInt32())
                    ret = ESValue(lval.asInt32() < rval.asInt32());

                if (lval.isNumber() && rval.isNumber())
                    ret = ESValue(lval.asNumber() < rval.asNumber());

                ASSERT(false);
                /*
                if (isJSString(v1) && isJSString(v2))
                    return !(asString(v2)->value(callFrame) < asString(v1)->value(callFrame));

                double n1;
                double n2;
                ESValue p1;
                ESValue p2;
                bool wasNotString1;
                bool wasNotString2;
                if (leftFirst) {
                    wasNotString1 = lval.getPrimitiveNumber(callFrame, n1, p1);
                    wasNotString2 = rval.getPrimitiveNumber(callFrame, n2, p2);
                } else {
                    wasNotString2 = rval.getPrimitiveNumber(callFrame, n2, p2);
                    wasNotString1 = lval.getPrimitiveNumber(callFrame, n1, p1);
                }

                if (wasNotString1 | wasNotString2)
                    return n1 < n2;
                return asString(p1)->value(callFrame) < asString(p2)->value(callFrame);
                */
            }
            break;

                      /*
                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.1
                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
                lval = lval->toPrimitive();
                rval = rval->toPrimitive();

                // TODO http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
                // string, NaN, zero, infinity, ...
                if (lval->isSmi() && rval->isSmi()) {
                    int lnum = lval->toSmi()->value();
                    int rnum = rval->toSmi()->value();
                    bool b;
                    if (oper == LessThan)                b = lnum < rnum;
                    else if (oper == LessThanOrEqual)    b = lnum <= rnum;
                    else if (oper == GreaterThan)        b = lnum > rnum;
                    else if (oper == GreaterThanOrEqual) b = lnum >= rnum;
                    else RELEASE_ASSERT_NOT_REACHED();
                    ret = b ? esTrue:esFalse;
                }
                else {
                    double lnum = lval->isSmi()? lval->toSmi()->value() : lval->toHeapObject()->toESNumber()->get();
                    double rnum = rval->isSmi()? rval->toSmi()->value() : rval->toHeapObject()->toESNumber()->get();
                    bool b;
                    if (oper == LessThan)                b = lnum < rnum;
                    else if (oper == LessThanOrEqual)    b = lnum <= rnum;
                    else if (oper == GreaterThan)        b = lnum > rnum;
                    else if (oper == GreaterThanOrEqual) b = lnum >= rnum;
                    else RELEASE_ASSERT_NOT_REACHED();
                    ret = b ? esTrue:esFalse;
                }
                */
                break;
            case BitwiseAnd:
                /*
                lval = lval->toInt32();
                rval = rval->toInt32();

                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.10
                if (lval->isSmi() && rval->isSmi()) {
                    ret = Smi::fromInt(lval->toSmi()->value() & rval->toSmi()->value());
                } else {
                    // TODO
                }
                */
                break;
            case LeftShift:
            case SignedRightShift:
            case UnsignedRightShift:
            {
                /*
                lval = lval->toInt32();
                rval = rval->toInt32();
                long long int rnum = rval->isSmi()? rval->toSmi()->value() : rval->toHeapObject()->toESNumber()->get();
                long long int lnum = lval->isSmi()? lval->toSmi()->value() : lval->toHeapObject()->toESNumber()->get();
                unsigned int shiftCount = ((unsigned int)rnum) & 0x1F;
                if(oper == LeftShift)
                    lnum <<= shiftCount;
                else if(oper == SignedRightShift)
                    lnum >>= shiftCount;
                else if(oper == UnsignedRightShift)
                    lnum = ((unsigned int)lnum) >> shiftCount;
                else
                    RELEASE_ASSERT_NOT_REACHED();

                if (lnum >= 40000000)
                    ret = ESNumber::create(lnum);
                else
                    ret = Smi::fromInt(lnum);
                    */
                break;
            }
            case Equals:
            /*
                if (lval->abstractEqualsTo(rval))
                    ret = ESBoolean::create(true);
                else
                    ret = ESBoolean::create(false);
                break;
            case NotEquals:
                if (lval->abstractEqualsTo(rval))
                    ret = ESBoolean::create(false);
                else
                    ret = ESBoolean::create(true);
                    */
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
