#ifndef ForInStatementNode_h
#define ForInStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class ForInStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
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
        ByteCodeGenerateContext newContext;
        newContext.m_offsetToBasePointer = context.m_offsetToBasePointer + 1;

        m_right->generateExpressionByteCode(codeBlock, newContext);
        codeBlock->pushCode(DuplicateTopOfStackValue(), this);
        codeBlock->pushCode(Push(ESValue(ESValue::ESUndefined)), this);
        codeBlock->pushCode(Equal(), this);
        codeBlock->pushCode(JumpIfTopOfStackValueIsTrue(SIZE_MAX), this);
        size_t exit1Pos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsTrue>();

        codeBlock->pushCode(DuplicateTopOfStackValue(), this);
        codeBlock->pushCode(Push(ESValue(ESValue::ESNull)), this);
        codeBlock->pushCode(Equal(), this);
        codeBlock->pushCode(JumpIfTopOfStackValueIsTrue(SIZE_MAX), this);
        size_t exit2Pos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsTrue>();

        codeBlock->pushCode(EnumerateObject(), this);
        size_t continuePosition = codeBlock->currentCodeSize();
        codeBlock->pushCode(EnumerateObjectKey(), this);

        codeBlock->pushCode(PushIntoTempStack(), this);
        m_left->generateResolveAddressByteCode(codeBlock, newContext);
        codeBlock->pushCode(PopFromTempStack(), this);
        m_left->generatePutByteCode(codeBlock, newContext);
        codeBlock->pushCode(Pop(), this);

        m_body->generateStatementByteCode(codeBlock, newContext);

        codeBlock->pushCode(Jump(continuePosition), this);
        size_t forInEnd = codeBlock->currentCodeSize();
        codeBlock->pushCode(EnumerateObjectEnd(), this);
        ASSERT(codeBlock->peekCode<EnumerateObjectKey>(continuePosition)->m_orgOpcode == EnumerateObjectKeyOpcode);
        codeBlock->peekCode<EnumerateObjectKey>(continuePosition)->m_forInEnd = forInEnd;

        newContext.consumeBreakPositions(codeBlock, forInEnd);
        newContext.consumeContinuePositions(codeBlock, continuePosition);
        newContext.propagateInformationTo(context);
        codeBlock->pushCode(Jump(SIZE_MAX), this);
        size_t jPos = codeBlock->lastCodePosition<Jump>();

        size_t exitPos = codeBlock->currentCodeSize();
        codeBlock->pushCode(Pop(), this);
        codeBlock->peekCode<JumpIfTopOfStackValueIsTrue>(exit1Pos)->m_jumpPosition = exitPos;
        codeBlock->peekCode<JumpIfTopOfStackValueIsTrue>(exit2Pos)->m_jumpPosition = exitPos;

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
