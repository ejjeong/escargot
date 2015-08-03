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
                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1
                lval = lval.toPrimitive();
                rval = rval.toPrimitive();
                if (lval.isESString() || rval.isESString()) {
                    InternalString lstr;
                    InternalString rstr;
                    if (lval.isESString())
                        lstr = lval.asESString()->string();
                    else {
                        //TODO use toESString
                        //lstr = lval.toESString().string();
                        lstr = lval.toInternalString();
                    }
                    if (rval.isESString())
                        rstr = rval.asESString()->string();
                    else {
                        //TODO use toESString
                        //rstr = rval.toESString().string();
                        rstr = rval.toInternalString();
                    }
                    ret = ESString::create((*lstr.string() + *rstr.string()).c_str());
                } else {
                    if(lval.isInt32() && rval.isInt32())
                        ret = ESValue(lval.asInt32() + rval.asInt32());
                    else
                        ret = ESValue(lval.toNumber() + rval.toNumber());
                }
                break;
             case Minus:
                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.2
                if (lval.isInt32() && rval.isInt32()) // no overflow
                    ret = ESValue(lval.asInt32() - rval.asInt32());
                else
                    ret = ESValue(lval.toNumber() - rval.toNumber());
                break;
            case Div: {
                double lvalue = lval.toNumber();
                double rvalue = rval.toNumber();
                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.5.2
                bool islNeg = lvalue < 0;
                bool isrNeg = rvalue < 0;
                bool islZero = lvalue == 0 || lvalue == -0.0;
                bool isrZero = rvalue == 0 || rvalue == -0.0;
                bool isNeg = (islNeg != isrNeg);
                if (lvalue == std::numeric_limits<double>::quiet_NaN() || rvalue == std::numeric_limits<double>::quiet_NaN()) ret = ESValue(std::numeric_limits<double>::quiet_NaN());
                else if (lvalue == std::numeric_limits<double>::infinity() || lvalue == -std::numeric_limits<double>::infinity()) {
                    if (rvalue == std::numeric_limits<double>::infinity() || rvalue == -std::numeric_limits<double>::infinity())
                        ret = ESValue(std::numeric_limits<double>::quiet_NaN());
                    else { // if rval is zero or nonzero finite value
                        if (isNeg) ret = ESValue(-std::numeric_limits<double>::infinity());
                        else       ret = ESValue(std::numeric_limits<double>::infinity());
                    }
                } else if (rvalue == std::numeric_limits<double>::infinity() || rvalue == -std::numeric_limits<double>::infinity()) {
                    if (isNeg) ret = ESValue(-0.0);
                    else       ret = ESValue(0);
                } else if (islZero) {
                    if (isrZero) ret = ESValue(std::numeric_limits<double>::quiet_NaN());
                    else {
                        if (isNeg) ret = ESValue(-0.0);
                        else       ret = ESValue(0);
                    }
                } else if (isrZero) {
                    if (isNeg) ret = ESValue(-std::numeric_limits<double>::infinity());
                    else       ret = ESValue(std::numeric_limits<double>::infinity());
                } else {
                    double lnum = lvalue;
                    double rnum = rvalue;
                    double result = lnum / rnum;

                    if (result == std::numeric_limits<double>::infinity())
                        ret = ESValue(std::numeric_limits<double>::infinity());
                    else if (result == -std::numeric_limits<double>::infinity())
                        ret = ESValue(-std::numeric_limits<double>::infinity());
                    else if (result == -0.0)
                        ret = ESValue(-0.0);
                    else
                        ret = ESValue(result);
                }
                }
                break;
            case LessThan:
            case LessThanOrEqual:
            case GreaterThan:
            case GreaterThanOrEqual:
            {
                /* http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.1
                 * http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5 */
                lval = lval.toPrimitive();
                rval = rval.toPrimitive();

                // TODO http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
                // string, NaN, zero, infinity, ...
                bool b;
                if(lval.isInt32() && rval.isInt32()) {
                    int lnum = lval.asInt32();
                    int rnum = rval.asInt32();
                    if (oper == LessThan)                b = lnum < rnum;
                    else if (oper == LessThanOrEqual)    b = lnum <= rnum;
                    else if (oper == GreaterThan)        b = lnum > rnum;
                    else if (oper == GreaterThanOrEqual) b = lnum >= rnum;
                    else RELEASE_ASSERT_NOT_REACHED();
                } else {
                    double lnum = lval.toNumber();
                    double rnum = rval.toNumber();
                    if (oper == LessThan)                b = lnum < rnum;
                    else if (oper == LessThanOrEqual)    b = lnum <= rnum;
                    else if (oper == GreaterThan)        b = lnum > rnum;
                    else if (oper == GreaterThanOrEqual) b = lnum >= rnum;
                    else RELEASE_ASSERT_NOT_REACHED();
                }
                ret = ESValue(b);
                break;
            }
            case BitwiseAnd:
            {
                int32_t lnum = lval.toInt32();
                int32_t rnum = rval.toInt32();

                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.10
                ret = ESValue(lnum & rnum);
                break;
            }
            case LeftShift:
            case SignedRightShift:
            case UnsignedRightShift:
            {
                long long int rnum = rval.toInt32();
                long long int lnum = lval.toInt32();
                unsigned int shiftCount = ((unsigned int)rnum) & 0x1F;
                if(oper == LeftShift)
                    lnum <<= shiftCount;
                else if(oper == SignedRightShift)
                    lnum >>= shiftCount;
                else if(oper == UnsignedRightShift)
                    lnum = ((unsigned int)lnum) >> shiftCount;
                else
                    RELEASE_ASSERT_NOT_REACHED();

                ret = ESValue(lnum);
                break;
            }
            case Equals:
                ret = ESValue(lval.abstractEqualsTo(rval));
                break;
            case NotEquals:
                ret = ESValue(!lval.abstractEqualsTo(rval));
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
