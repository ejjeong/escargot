#ifndef UnaryExpressionVoidNode_h
#define UnaryExpressionVoidNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionVoidNode : public ExpressionNode {
public:
    friend class ScriptParser;
    UnaryExpressionVoidNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionVoid)
    {
        m_argument = argument;
    }

    /*
    ESValue executeExpression(ESVMInstance* instance)
    {
        // www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-minus-operator
        ESValue v;
        try {
            v = m_argument->executeExpression(instance);
        } catch(const ESValue& e) {
            if ((m_argument->type() == Identifier) && e.isESPointer() && e.asESPointer()->isESObject() && e.asESPointer()->asESObject()->constructor() == ESValue(instance->globalObject()->referenceError())) {

            } else {
                throw e;
            }
        }

        if (v.isNumber())
            return ESValue();
        else
            RELEASE_ASSERT_NOT_REACHED();
    }
    */


    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(UnaryVoid(), context, this);
    }

protected:
    Node* m_argument;
};

}

#endif
