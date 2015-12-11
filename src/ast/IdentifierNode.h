#ifndef IdentifierNode_h
#define IdentifierNode_h

#include "Node.h"
#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

// interface Identifier <: Node, Expression, Pattern {
class IdentifierNode : public Node {
public:
    friend class ScriptParser;
    IdentifierNode(const InternalAtomicString& name)
        : Node(NodeType::Identifier)
    {
        m_name = name;
        m_canUseFastAccess = false;
        m_fastAccessIndex = SIZE_MAX;
        m_fastAccessUpIndex = SIZE_MAX;
        m_canUseGlobalFastAccess = false;
        m_globalFastAccessIndex = SIZE_MAX;
    }

    virtual NodeType type() { return NodeType::Identifier; }

    IdentifierNode* clone()
    {
        IdentifierNode* nd = new IdentifierNode(m_name);
#ifndef NDEBUG
        nd->m_sourceLocation = m_sourceLocation;
#endif
        return nd;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_canUseFastAccess) {
            if (codeBlock->m_needsHeapAllocatedVariableStorage) {
                codeBlock->pushCode(GetByIndexWithActivation(m_fastAccessIndex, m_fastAccessUpIndex), context, this);
#ifndef NDEBUG
                codeBlock->peekCode<GetByIndexWithActivation>(codeBlock->lastCodePosition<GetByIndexWithActivation>())->m_name = m_name;
#endif
            } else {
                if (m_fastAccessUpIndex == 0) {
                    codeBlock->pushCode(GetByIndex(m_fastAccessIndex), context, this);
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
        } else if (m_canUseGlobalFastAccess) {
            codeBlock->pushCode(GetByGlobalIndex(m_globalFastAccessIndex, m_name.string()), context, this);
        } else {
            if (m_name == strings->arguments) {
                codeBlock->pushCode(GetArgumentsObject(), context, this);
            } else {
                codeBlock->pushCode(GetById(m_name), context, this);
            }
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
        if (m_canUseFastAccess) {
            if (codeBlock->m_needsHeapAllocatedVariableStorage) {
                codeBlock->pushCode(SetByIndexWithActivation(m_fastAccessIndex, m_fastAccessUpIndex), context, this);
            } else {
                if (m_fastAccessUpIndex == 0) {
                    codeBlock->pushCode(SetByIndex(m_fastAccessIndex), context, this);
                } else
                    codeBlock->pushCode(SetByIndexWithActivation(m_fastAccessIndex, m_fastAccessUpIndex), context, this);
            }
        } else if (m_canUseGlobalFastAccess) {
            codeBlock->pushCode(SetByGlobalIndex(m_globalFastAccessIndex, m_name.string()), context, this);
        } else {
            if (m_name == strings->arguments) {
                codeBlock->pushCode(SetArgumentsObject(), context, this);
            } else {
                codeBlock->pushCode(SetById(m_name), context, this);
            }
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 12;
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

    void setGlobalFastAccessIndex(size_t index)
    {
        m_canUseGlobalFastAccess = true;
        m_globalFastAccessIndex = index;
    }

    size_t globalFastAccessIndex()
    {
        return m_globalFastAccessIndex;
    }

    virtual bool isIdentifier()
    {
        return true;
    }


protected:
    InternalAtomicString m_name;

    bool m_canUseFastAccess;
    size_t m_fastAccessIndex;
    size_t m_fastAccessUpIndex;

    bool m_canUseGlobalFastAccess;
    size_t m_globalFastAccessIndex;
};

}

#endif
