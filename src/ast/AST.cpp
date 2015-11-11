#include "Escargot.h"
#include "AST.h"

namespace escargot {

bool findRightAfterExpression(Node* node, NodeType tp)
{
    // TODO this is experimental implement
    // we should re-implement this function
    if (node->type() == tp) {
        return true;
    } else {
        if (node->type() == SequenceExpression) {
            return findRightAfterExpression(((SequenceExpressionNode *)node)->expressions().back(), tp);
        }
    }

    return false;
}

}
