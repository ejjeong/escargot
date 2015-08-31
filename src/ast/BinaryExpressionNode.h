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
        StrictEquals, //"==="
        NotStrictEquals, //"!=="

        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.10
        // Binary Bitwise operators
        BitwiseAnd, //"&"
        BitwiseXor, //"^"
        BitwiseOr,  //"|"

        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.11
        // Binary Logical Operators
        LogicalAnd, //"&&"
        LogicalOr,  //"||"
        // TODO
    };

    BinaryExpressionNode(Node *left, Node* right, ESString* oper)
            : ExpressionNode(NodeType::BinaryExpression)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;

        // Additive Operators
        if (*oper == u"+")
            m_operator = Plus;
        else if (*oper == u"-")
            m_operator = Minus;

        // Bitwise Shift Operators
        else if (*oper == u"<<")
            m_operator = LeftShift;
        else if (*oper == u">>")
            m_operator = SignedRightShift;
        else if (*oper == u">>>")
            m_operator = UnsignedRightShift;

        // Multiplicative Operators
        else if (*oper == u"*")
            m_operator = Mult;
        else if (*oper == u"/")
            m_operator = Div;
        else if (*oper == u"%")
            m_operator = Mod;

        // Relational Operators
        else if (*oper == u"<")
            m_operator = LessThan;
        else if (*oper == u">")
            m_operator = GreaterThan;
        else if (*oper == u"<=")
            m_operator = LessThanOrEqual;
        else if (*oper == u">=")
            m_operator = GreaterThanOrEqual;

        // Equality Operators
        else if (*oper == u"==")
            m_operator = Equals;
        else if (*oper == u"!=")
            m_operator = NotEquals;
        else if (*oper == u"===")
            m_operator = StrictEquals;
        else if (*oper == u"!==")
            m_operator = NotStrictEquals;

        // Binary Bitwise Operator
        else if (*oper == u"&")
            m_operator = BitwiseAnd;
        else if (*oper == u"^")
            m_operator = BitwiseXor;
        else if (*oper == u"|")
            m_operator = BitwiseOr;

        else if (*oper == u"||")
            m_operator = LogicalOr;
        else if (*oper == u"&&")
            m_operator = LogicalAnd;
        // TODO
        else
            RELEASE_ASSERT_NOT_REACHED();
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue lval = m_left->execute(instance);
        ESValue rval = m_right->execute(instance);
        return execute(instance, lval, rval, m_operator);
    }

    ALWAYS_INLINE static ESValue execute(ESVMInstance* instance, ESValue lval, ESValue rval, BinaryExpressionOperator oper) {
        ESValue ret(ESValue::ESForceUninitialized);
        switch(oper) {
            case Plus:
                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1
                lval = lval.toPrimitive();
                rval = rval.toPrimitive();
                if (lval.isESString() || rval.isESString()) {
                    ESString* lstr;
                    ESString* rstr;

                    lstr = lval.toString();
                    rstr = rval.toString();
                    ret = ESString::concatTwoStrings(lstr, rstr);
                } else {
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
                    }
                    else
                        ret = ESValue(lval.toNumber() + rval.toNumber());
                }
                break;
             case Minus:
                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.2
                if (lval.isInt32() && rval.isInt32()) {
                    int a = lval.asInt32(), b = rval.asInt32();
                    if (UNLIKELY((a > 0 && b < 0 && b < a - std::numeric_limits<int32_t>::max()))) {
                        //overflow
                        ret = ESValue((double)lval.asInt32() - (double)rval.asInt32());
                    } else if (UNLIKELY(a < 0 && b > 0 && b > a - std::numeric_limits<int32_t>::min())) {
                        //underflow
                        ret = ESValue((double)lval.asInt32() - (double)rval.asInt32());
                    } else {
                        ret = ESValue(lval.asInt32() - rval.asInt32());
                    }
                }
                else
                    ret = ESValue(lval.toNumber() - rval.toNumber());
                break;
             case Mult: {
                 // http://www.ecma-international.org/ecma-262/5.1/#sec-11.5.1
                 double lvalue = lval.toNumber();
                 double rvalue = rval.toNumber();
                 ret = ESValue(lvalue*rvalue);
                 /*
                 bool islNeg = lvalue < 0;
                 bool isrNeg = rvalue < 0;
                 bool islZero = lvalue == 0 || lvalue == -0.0;
                 bool isrZero = rvalue == 0 || rvalue == -0.0;
                 bool isNeg = (islNeg != isrNeg);

                 if (lvalue == std::numeric_limits<double>::quiet_NaN() || rvalue == std::numeric_limits<double>::quiet_NaN())
                     ret = ESValue(std::numeric_limits<double>::quiet_NaN());
                 else if ((lvalue == std::numeric_limits<double>::infinity() || lvalue == -std::numeric_limits<double>::infinity()) && isrZero) {
                     ret = ESValue(std::numeric_limits<double>::quiet_NaN());
                 } else if ((rvalue == std::numeric_limits<double>::infinity() || rvalue == -std::numeric_limits<double>::infinity()) && islZero) {
                     ret = ESValue(-std::numeric_limits<double>::quiet_NaN());
                 } else if (
                         (lvalue == std::numeric_limits<double>::infinity() || lvalue == -std::numeric_limits<double>::infinity()) &&
                         (rvalue == std::numeric_limits<double>::infinity() || rvalue == -std::numeric_limits<double>::infinity())) {
                     if(islNeg && isrNeg)
                         ret = ESValue(std::numeric_limits<double>::infinity());
                     else if(islNeg || isrNeg)
                         ret = ESValue(-std::numeric_limits<double>::infinity());
                     else
                         ret = ESValue(std::numeric_limits<double>::infinity());
                 } else if (
                         (lvalue == std::numeric_limits<double>::infinity() || lvalue == -std::numeric_limits<double>::infinity()) ||
                         (rvalue == std::numeric_limits<double>::infinity() || rvalue == -std::numeric_limits<double>::infinity())) {
                     if(islNeg && isrNeg)
                          ret = ESValue(std::numeric_limits<double>::infinity());
                      else if(islNeg || isrNeg)
                          ret = ESValue(-std::numeric_limits<double>::infinity());
                      else
                          ret = ESValue(std::numeric_limits<double>::infinity());
                 } else {
                     ret = ESValue(lvalue * rvalue);
                 }
                 */
                 }
                 break;
            case Div: {
                double lvalue = lval.toNumber();
                double rvalue = rval.toNumber();
                ret = ESValue(lvalue/rvalue);
                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.5.2
                /*
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
                */
              }
                break;
            case Mod: {
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
                break;
            }
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
                if (lval.isESString() || rval.isESString()) {
                    ESString* lstr;
                    ESString* rstr;

                    lstr = lval.toString();
                    rstr = rval.toString();

                    if (oper == LessThan)                b = lstr->string() < rstr->string();
                    else if (oper == LessThanOrEqual)    b = lstr->string() <= rstr->string();
                    else if (oper == GreaterThan)        b = lstr->string() > rstr->string();
                    else if (oper == GreaterThanOrEqual) b = lstr->string() >= rstr->string();
                    else RELEASE_ASSERT_NOT_REACHED();

                } else {
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
                }

                if(b)
                    ret = ESValue(ESValue::ESTrueTag::ESTrue);
                else
                    ret = ESValue(ESValue::ESFalseTag::ESFalse);
                break;
            }
            case BitwiseAnd:
            case BitwiseXor:
            case BitwiseOr:
            {
                int32_t lnum = lval.toInt32();
                int32_t rnum = rval.toInt32();

                // http://www.ecma-international.org/ecma-262/5.1/#sec-11.10
                int32_t r;
                if (oper == BitwiseAnd)         r = lnum & rnum;
                else if (oper == BitwiseXor)    r = lnum ^ rnum;
                else if (oper == BitwiseOr)     r = lnum | rnum;
                else RELEASE_ASSERT_NOT_REACHED();
                ret = ESValue(r);
                break;
            }
            case LeftShift:
            case SignedRightShift:
            case UnsignedRightShift:
            {
                int32_t rnum = rval.toInt32();
                int32_t lnum = lval.toInt32();
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
            {
                bool b = lval.abstractEqualsTo(rval);
                if(b)
                    ret = ESValue(ESValue::ESTrueTag::ESTrue);
                else
                    ret = ESValue(ESValue::ESFalseTag::ESFalse);
                break;
            }
            case NotEquals:
            {
                bool b = !lval.abstractEqualsTo(rval);
                if(b)
                    ret = ESValue(ESValue::ESTrueTag::ESTrue);
                else
                    ret = ESValue(ESValue::ESFalseTag::ESFalse);
                break;
            }
            case StrictEquals:
            {
                bool b = lval.equalsTo(rval);
                if(b)
                    ret = ESValue(ESValue::ESTrueTag::ESTrue);
                else
                    ret = ESValue(ESValue::ESFalseTag::ESFalse);
                break;
            }
            case NotStrictEquals:
            {
                bool b = !lval.equalsTo(rval);
                if(b)
                    ret = ESValue(ESValue::ESTrueTag::ESTrue);
                else
                    ret = ESValue(ESValue::ESFalseTag::ESFalse);
                break;
            }
            case LogicalAnd:
                if (lval.toBoolean() == false) ret = lval;
                else ret = rval;
                break;
            case LogicalOr:
                if (lval.toBoolean() == true) ret = lval;
                else ret = rval;
                break;
            default:
                // TODO
                printf("unsupport operator is->%d\n",(int)oper);
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
