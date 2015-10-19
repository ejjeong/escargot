#ifndef IdentifierNode_h
#define IdentifierNode_h

#include "Node.h"
#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

//interface Identifier <: Node, Expression, Pattern {
class IdentifierNode : public Node {
public:
    friend class ESScriptParser;
    IdentifierNode(const InternalAtomicString& name)
            : Node(NodeType::Identifier)
    {
        m_name = name;
        m_canUseFastAccess = false;
        m_fastAccessIndex = SIZE_MAX;
        m_fastAccessUpIndex = SIZE_MAX;
    }
    IdentifierNode* clone() {
        IdentifierNode* nd = new IdentifierNode(m_name);
        nd->m_sourceLocation = m_sourceLocation;
        return nd;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if(m_canUseFastAccess) {
            if(codeBlock->m_needsActivation) {
                codeBlock->pushCode(GetByIndexWithActivation(m_fastAccessIndex, m_fastAccessUpIndex), context, this);
#ifndef NDEBUG
                codeBlock->peekCode<GetByIndexWithActivation>(codeBlock->lastCodePosition<GetByIndexWithActivation>())->m_name = m_name;
#endif
            } else {
                if(m_fastAccessUpIndex == 0) {
                    updateNodeIndex(context);
                    codeBlock->pushCode(GetByIndex(m_fastAccessIndex), context, this);
                    WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
#ifndef NDEBUG
                    codeBlock->peekCode<GetByIndex>(codeBlock->lastCodePosition<GetByIndex>())->m_name = m_name;
#endif
                } else {
                    codeBlock->pushCode(GetByIndexWithActivation(m_fastAccessIndex, m_fastAccessUpIndex), context, this);
#ifndef NDEBUG
                    codeBlock->peekCode<GetByIndexWithActivation>(codeBlock->lastCodePosition<GetByIndexWithActivation>())->m_name = m_name;
#endif
                }
            }
        } else {
            updateNodeIndex(context);
            if(m_name == strings->arguments) {
                codeBlock->pushCode(GetArgumentsObject(), context, this);
            } else {
                codeBlock->pushCode(GetById(m_name), context, this);
            }
            WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
        }
    }

    virtual void generateResolveAddressByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
    }

    virtual void generateReferenceResolvedAddressByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        generateExpressionByteCode(codeBlock, context);
    }


    virtual void generatePutByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if(m_canUseFastAccess) {
            if(codeBlock->m_needsActivation) {
                codeBlock->pushCode(SetByIndexWithActivation(m_fastAccessIndex, m_fastAccessUpIndex), context, this);
            } else {
                if(m_fastAccessUpIndex == 0) {
//                    updateNodeIndex(context);
                    codeBlock->pushCode(SetByIndex(m_fastAccessIndex), context, this);
//                    WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
                } else
                    codeBlock->pushCode(SetByIndexWithActivation(m_fastAccessIndex, m_fastAccessUpIndex), context, this);
            }
        } else {
            if(m_name == strings->arguments) {
                codeBlock->pushCode(SetArgumentsObject(), context, this);
            } else {
                codeBlock->pushCode(SetById(m_name), context, this);
            }
        }
    }

    const InternalAtomicString& name()
    {
        return m_name;
    }

    ESString* nonAtomicName() const
    {
        return m_name.string();
    }

    void setFastAccessIndex(size_t upIndex, size_t index)
    {
        m_canUseFastAccess = true;
        m_fastAccessIndex = index;
        m_fastAccessUpIndex = upIndex;
    }

    bool canUseFastAccess()
    {
        return m_canUseFastAccess;
    }

    size_t fastAccessIndex()
    {
        return m_fastAccessIndex;
    }

    size_t fastAccessUpIndex()
    {
        return m_fastAccessUpIndex;
    }

protected:
    InternalAtomicString m_name;

    bool m_canUseFastAccess;
    size_t m_fastAccessIndex;
    size_t m_fastAccessUpIndex;
};

}

#endif
