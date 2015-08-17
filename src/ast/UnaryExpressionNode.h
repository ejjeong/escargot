#ifndef UnaryExpressionNode_h
#define UnaryExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionNode : public ExpressionNode {
public:
    enum Operator {
        Plus,
        Minus,
        BitwiseNot,
        LogicalNot,
        TypeOf,
    };
    friend class ESScriptParser;
    UnaryExpressionNode(Node* argument, ESString* oper)
        : ExpressionNode(NodeType::UnaryExpression)
    {
        m_argument = argument;
        if(*oper == L"+") {
            m_operator = Plus;
        } else if(*oper == L"-") {
            m_operator = Minus;
        } else if(*oper == L"~") {
            m_operator = BitwiseNot;
        } else if(*oper == L"!") {
            m_operator = LogicalNot;
        } else if(*oper == L"typeof") {
            m_operator = TypeOf;
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    ESValue execute(ESVMInstance* instance)
    {

        if(m_operator == Plus) {
            //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-plus-operator
            return ESValue(m_argument->execute(instance).toNumber());
        } else if(m_operator == Minus) {
            //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-minus-operator
            return ESValue(-m_argument->execute(instance).toNumber());
        } else if(m_operator == BitwiseNot) {
            //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-bitwise-not-operator
            return ESValue(~m_argument->execute(instance).toInt32());
        } else if(m_operator == LogicalNot) {
            //www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-minus-operator
            return ESValue(!m_argument->execute(instance).toBoolean());
        } else if(m_operator == TypeOf) {
            //www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-minus-operator
            ESValue v = m_argument->execute(instance);
            if(v.isUndefined())
                return strings->undefined;
            else if(v.isNull())
                return strings->null;
            else if(v.isBoolean())
                return strings->boolean;
            else if(v.isNumber())
                return strings->number;
            else if(v.isESString())
                return strings->string;
            else if(v.isESPointer()) {
                ESPointer* p = v.asESPointer();
                if(p->isESFunctionObject()) {
                    return strings->function;
                } else {
                    return strings->object;
                }
            }
            else
                RELEASE_ASSERT_NOT_REACHED();
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
protected:
    Operator m_operator;
    Node* m_argument;
};

}

#endif
