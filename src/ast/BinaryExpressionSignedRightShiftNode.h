#ifndef BinaryExpressionSignedRightShiftNode_h
#define BinaryExpressionSignedRightShiftNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionSignedRightShiftNode : public ExpressionNode {
public:
    BinaryExpressionSignedRightShiftNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionSignedRightShift)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue lval = m_left->execute(instance);
        ESValue rval = m_right->execute(instance);
        int32_t rnum = rval.toInt32();
        int32_t lnum = lval.toInt32();
        unsigned int shiftCount = ((unsigned int)rnum) & 0x1F;
        lnum >>= shiftCount;
        return ESValue(lnum);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
