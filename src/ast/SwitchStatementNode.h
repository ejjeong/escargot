#ifndef SwitchStatementNode_h
#define SwitchStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"
#include "SwitchCaseNode.h"

namespace escargot {

class SwitchStatementNode : public StatementNode, public ControlFlowNode {
public:
    friend class ESScriptParser;
    SwitchStatementNode(Node* discriminant, StatementNodeVector&& casesA, Node* deflt, StatementNodeVector&& casesB, bool lexical)
            : StatementNode(NodeType::SwitchStatement)
    {
        m_discriminant = (ExpressionNode*) discriminant;
        m_casesA = casesA;
        m_default = (StatementNode*) deflt;
        m_casesB = casesB;
        m_lexical = lexical;
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue input = m_discriminant->execute(instance);
        instance->currentExecutionContext()->setJumpPositionAndExecute([&](){
            bool found = false;
            unsigned i;
            for (i = 0; i < m_casesA.size(); i++) {
                SwitchCaseNode* caseNode = (SwitchCaseNode*) m_casesA[i];
                if (!found) {
                    ESValue clauseSelector = caseNode->m_test->execute(instance);
                    if (input.equalsTo(clauseSelector)) {
                        found = true;
                    }
                }
                if (found)
                    caseNode->execute(instance);
            }
            bool foundInB = false;
            if (!found) {
                for (i = 0; i < m_casesB.size(); i++) {
                    if (foundInB)
                        break;
                    SwitchCaseNode* caseNode = (SwitchCaseNode*) m_casesB[i];
                    ESValue clauseSelector = caseNode->m_test->execute(instance);
                    if (input.equalsTo(clauseSelector)) {
                        foundInB = true;
                        caseNode->execute(instance);
                    }
                }
            }
            if (!foundInB && m_default)
                m_default->execute(instance);

            for (; i < m_casesB.size(); i++) {
                SwitchCaseNode* caseNode = (SwitchCaseNode*) m_casesB[i];
                caseNode->execute(instance);
            }
        });
        return ESValue();
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
