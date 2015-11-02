#ifndef SwitchStatementNode_h
#define SwitchStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"
#include "SwitchCaseNode.h"

namespace escargot {

class SwitchStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    SwitchStatementNode(Node* discriminant, StatementNodeVector&& casesA, Node* deflt, StatementNodeVector&& casesB, bool lexical)
        : StatementNode(NodeType::SwitchStatement)
    {
        m_discriminant = (ExpressionNode*) discriminant;
        m_casesA = casesA;
        m_default = (StatementNode*) deflt;
        m_casesB = casesB;
        m_lexical = lexical;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        ByteCodeGenerateContext newContext(context);
        m_discriminant->generateExpressionByteCode(codeBlock, newContext);

        std::vector<size_t> jumpCodePerCaseNodePosition;
        for(unsigned i = 0; i < m_casesB.size() ; i ++) {
            SwitchCaseNode* caseNode = (SwitchCaseNode*) m_casesB[i];
            codeBlock->pushCode(DuplicateTopOfStackValue(), newContext, this);
            caseNode->m_test->generateExpressionByteCode(codeBlock, newContext);
            codeBlock->pushCode(StrictEqual(), newContext, this);
            jumpCodePerCaseNodePosition.push_back(codeBlock->currentCodeSize());
            codeBlock->pushCode(JumpAndPopIfTopOfStackValueIsTrue(SIZE_MAX), newContext, this);
        }
        for(unsigned i = 0; i < m_casesA.size() ; i ++) {
            SwitchCaseNode* caseNode = (SwitchCaseNode*) m_casesA[i];
            codeBlock->pushCode(DuplicateTopOfStackValue(), newContext, this);
            caseNode->m_test->generateExpressionByteCode(codeBlock, newContext);
            codeBlock->pushCode(StrictEqual(), newContext, this);
            jumpCodePerCaseNodePosition.push_back(codeBlock->currentCodeSize());
            codeBlock->pushCode(JumpAndPopIfTopOfStackValueIsTrue(SIZE_MAX), newContext, this);
        }
        size_t jmpToDefault = SIZE_MAX;
        codeBlock->pushCode(Pop(), newContext, this);
        jmpToDefault = codeBlock->currentCodeSize();
        codeBlock->pushCode(Jump(SIZE_MAX), newContext, this);

        size_t caseIdx = 0;
        for(unsigned i = 0; i < m_casesB.size() ; i ++) {
            SwitchCaseNode* caseNode = (SwitchCaseNode*) m_casesB[i];
            codeBlock->peekCode<JumpAndPopIfTopOfStackValueIsTrue>(jumpCodePerCaseNodePosition[caseIdx++])->m_jumpPosition = codeBlock->currentCodeSize();
            caseNode->generateStatementByteCode(codeBlock, newContext);
        }
        if(m_default) {
            codeBlock->peekCode<Jump>(jmpToDefault)->m_jumpPosition = codeBlock->currentCodeSize();
            m_default->generateStatementByteCode(codeBlock, newContext);
        }
        for(unsigned i = 0; i < m_casesA.size() ; i ++) {
            SwitchCaseNode* caseNode = (SwitchCaseNode*) m_casesA[i];
            codeBlock->peekCode<JumpAndPopIfTopOfStackValueIsTrue>(jumpCodePerCaseNodePosition[caseIdx++])->m_jumpPosition = codeBlock->currentCodeSize();
            caseNode->generateStatementByteCode(codeBlock, newContext);
        }
        size_t breakPos = codeBlock->currentCodeSize();
        newContext.consumeBreakPositions(codeBlock, breakPos);
        newContext.m_positionToContinue = context.m_positionToContinue;
        newContext.propagateInformationTo(context);

        if(!m_default) {
            codeBlock->peekCode<Jump>(jmpToDefault)->m_jumpPosition = codeBlock->currentCodeSize();
        }
    }

protected:
    ExpressionNode* m_discriminant;
    StatementNodeVector m_casesA;
    StatementNode* m_default;
    StatementNodeVector m_casesB;
    bool m_lexical;
};

}

#endif


