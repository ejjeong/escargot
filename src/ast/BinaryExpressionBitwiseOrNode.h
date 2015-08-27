#ifndef BinaryExpressionBitwiseOrNode_h
#define BinaryExpressionBitwiseOrNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionBitwiseOrNode: public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionBitwiseOrNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionBitwiseOr)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        int32_t lnum = m_left->execute(instance).toInt32();
        int32_t rnum = m_right->execute(instance).toInt32();
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.10
        return ESValue(lnum | rnum);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
