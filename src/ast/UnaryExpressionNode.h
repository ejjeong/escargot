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
    };
    friend class ESScriptParser;
    UnaryExpressionNode(Node* argument, const InternalString& oper)
        : ExpressionNode(NodeType::UnaryExpression)
    {
        m_argument = argument;
        if(oper == L"+") {
            m_operator = Plus;
        } else if(oper == L"-") {
            m_operator = Minus;
        } else if(oper == L"~") {
            m_operator = BitwiseNot;
        } else if(oper == L"!") {
            m_operator = LogicalNot;
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual ESValue execute(ESVMInstance* instance)
    {

        if(m_operator == Plus) {
            //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-plus-operator
            return ESValue(m_argument->execute(instance).ensureValue().toNumber());
        } else if(m_operator == Minus) {
            //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-minus-operator
            return ESValue(-m_argument->execute(instance).ensureValue().toNumber());
        } else if(m_operator == BitwiseNot) {
            //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-bitwise-not-operator
            return ESValue(~m_argument->execute(instance).ensureValue().toInt32());
        } else if(m_operator == LogicalNot) {
            //www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-minus-operator
            return ESValue(!m_argument->execute(instance).ensureValue().toBoolean());
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
