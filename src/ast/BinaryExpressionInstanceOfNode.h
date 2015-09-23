#ifndef BinaryExpressionInstanceOfNode_h
#define BinaryExpressionInstanceOfNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionInstanceOfNode : public ExpressionNode {
public:
    BinaryExpressionInstanceOfNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionInstanceOf)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    /*
    ESValue executeExpression(ESVMInstance* instance)
    {
        ESValue lval = m_left->executeExpression(instance);
        ESValue rval = m_right->executeExpression(instance);

        if (rval.isESPointer() && rval.asESPointer()->isESFunctionObject() &&
                lval.isESPointer() && lval.asESPointer()->isESObject()) {
            ESFunctionObject* C = rval.asESPointer()->asESFunctionObject();
            ESValue P = C->protoType();
            ESValue O = lval.asESPointer()->asESObject()->__proto__();
            if (P.isESPointer() && P.asESPointer()->isESObject()) {
                while (!O.isUndefinedOrNull()) {
                    if (P == O)
                        return ESValue(true);
                    O = O.asESPointer()->asESObject()->__proto__();
                  }
            } else {
                throw ReferenceError::create(ESString::create(u""));
              }
         }

        return ESValue(false);
    }
    */

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(InstanceOf(), this);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
