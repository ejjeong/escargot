#ifndef ForInStatementNode_h
#define ForInStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class ForInStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    ForInStatementNode(Node *left, Node *right, Node *body, bool each)
            : StatementNode(NodeType::ForInStatement)
    {
        m_left = (ExpressionNode*) left;
        m_right = (ExpressionNode*) right;
        m_body = (StatementNode*) body;
        m_each = each;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        ByteCodeGenerateContext newContext(context);
        newContext.m_offsetToBasePointer = context.m_offsetToBasePointer + 1;

        m_right->generateExpressionByteCode(codeBlock, newContext);
        codeBlock->pushCode(DuplicateTopOfStackValue(), newContext, this);
        codeBlock->pushCode(Push(ESValue(ESValue::ESUndefined)), newContext, this);
        codeBlock->pushCode(Equal(), newContext, this);
        codeBlock->pushCode(JumpAndPopIfTopOfStackValueIsTrue(SIZE_MAX), newContext, this);
        size_t exit1Pos = codeBlock->lastCodePosition<JumpAndPopIfTopOfStackValueIsTrue>();

        codeBlock->pushCode(DuplicateTopOfStackValue(), newContext, this);
        codeBlock->pushCode(Push(ESValue(ESValue::ESNull)), newContext, this);
        codeBlock->pushCode(Equal(), newContext, this);
        codeBlock->pushCode(JumpAndPopIfTopOfStackValueIsTrue(SIZE_MAX), newContext, this);
        size_t exit2Pos = codeBlock->lastCodePosition<JumpAndPopIfTopOfStackValueIsTrue>();

        codeBlock->pushCode(EnumerateObject(), newContext, this);
        size_t continuePosition = codeBlock->currentCodeSize();
        codeBlock->pushCode(EnumerateObjectKey(), newContext, this);

        codeBlock->pushCode(PushIntoTempStack(), newContext, this);
        m_left->generateResolveAddressByteCode(codeBlock, newContext);
        codeBlock->pushCode(PopFromTempStack(), newContext, this);
        m_left->generatePutByteCode(codeBlock, newContext);
        codeBlock->pushCode(Pop(), newContext, this);

        m_body->generateStatementByteCode(codeBlock, newContext);

        codeBlock->pushCode(Jump(continuePosition), newContext, this);
        size_t forInEnd = codeBlock->currentCodeSize();
        //codeBlock->pushCode(EnumerateObjectEnd(), this);
        codeBlock->pushCode(Pop(), newContext, this);
        ASSERT(codeBlock->peekCode<EnumerateObjectKey>(continuePosition)->m_orgOpcode == EnumerateObjectKeyOpcode);
        codeBlock->peekCode<EnumerateObjectKey>(continuePosition)->m_forInEnd = forInEnd;

        newContext.consumeBreakPositions(codeBlock, forInEnd);
        newContext.consumeContinuePositions(codeBlock, continuePosition);
        newContext.m_positionToContinue = continuePosition;
        newContext.propagateInformationTo(newContext);
        codeBlock->pushCode(Jump(SIZE_MAX), newContext, this);
        size_t jPos = codeBlock->lastCodePosition<Jump>();

        size_t exitPos = codeBlock->currentCodeSize();
        codeBlock->peekCode<JumpAndPopIfTopOfStackValueIsTrue>(exit1Pos)->m_jumpPosition = exitPos;
        codeBlock->peekCode<JumpAndPopIfTopOfStackValueIsTrue>(exit2Pos)->m_jumpPosition = exitPos;

        codeBlock->peekCode<Jump>(jPos)->m_jumpPosition = codeBlock->currentCodeSize();

    }


protected:
    ExpressionNode *m_left;
    ExpressionNode *m_right;
    StatementNode *m_body;
    bool m_each;
};

}

#endif
