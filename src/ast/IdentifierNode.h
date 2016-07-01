/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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
        m_flags.m_canUseFastAccess = false;
        m_flags.m_canUseGlobalFastAccess = false;
        m_flags.m_isFastAccessIndexIndicatesHeapIndex = false;
        m_flags.m_onlySearchGlobal = false;
        m_flags.m_fastAccessIndexIndicatesImmutableBinding = false;
        m_fastAccessIndex = SIZE_MAX;
        m_fastAccessUpIndex = SIZE_MAX;
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
        if (canUseFastAccess()) {
            if (m_fastAccessUpIndex) {
                ASSERT(m_flags.m_isFastAccessIndexIndicatesHeapIndex);
                codeBlock->pushCode(GetByIndexInUpperContextHeap(m_fastAccessIndex, m_fastAccessUpIndex), context, this);
#ifndef NDEBUG
                codeBlock->peekCode<GetByIndexInUpperContextHeap>(codeBlock->lastCodePosition<GetByIndexInUpperContextHeap>())->m_name = m_name;
#endif
            } else {
                if (m_flags.m_isFastAccessIndexIndicatesHeapIndex) {
                    codeBlock->pushCode(GetByIndexInHeap(m_fastAccessIndex), context, this);
#ifndef NDEBUG
                    codeBlock->peekCode<GetByIndexInHeap>(codeBlock->lastCodePosition<GetByIndexInHeap>())->m_name = m_name;
#endif
                } else {
                    codeBlock->pushCode(GetByIndex(m_fastAccessIndex), context, this);
#ifndef NDEBUG
                    codeBlock->peekCode<GetByIndex>(codeBlock->lastCodePosition<GetByIndex>())->m_name = m_name;
#endif
                }
            }
        } else if (canUseGlobalFastAccess()) {
            codeBlock->pushCode(GetByGlobalIndex(m_fastAccessIndex, m_name.string()), context, this);
        } else {
            if (m_name == strings->arguments && !context.m_isGlobalScope && !context.m_hasArgumentsBinding) {
                codeBlock->pushCode(GetArgumentsObject(), context, this);
            } else {
                codeBlock->pushCode(GetById(m_name, m_flags.m_onlySearchGlobal), context, this);
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
        if (canUseFastAccess()) {
            if (fastAccessIndexIndicatesImmutableBinding()) {
                if (codeBlock->m_isStrict) {
                    codeBlock->pushCode(ThrowStatic(ESErrorObject::Code::TypeError, ESString::create(u"Attempted to assign to readonly property.")), context, this);
                }
            } else if (m_fastAccessUpIndex) {
                ASSERT(m_flags.m_isFastAccessIndexIndicatesHeapIndex);
                codeBlock->pushCode(SetByIndexInUpperContextHeap(m_fastAccessIndex, m_fastAccessUpIndex), context, this);
            } else {
                if (m_flags.m_isFastAccessIndexIndicatesHeapIndex)
                    codeBlock->pushCode(SetByIndexInHeap(m_fastAccessIndex), context, this);
                else
                    codeBlock->pushCode(SetByIndex(m_fastAccessIndex), context, this);
            }
        } else if (canUseGlobalFastAccess()) {
            codeBlock->pushCode(SetByGlobalIndex(m_fastAccessIndex, m_name.string()), context, this);
        } else {
            if (m_name == strings->arguments && !context.m_isGlobalScope && !context.m_hasArgumentsBinding) {
                codeBlock->pushCode(SetArgumentsObject(), context, this);
            } else {
                codeBlock->pushCode(SetById(m_name, m_flags.m_onlySearchGlobal), context, this);
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
        m_flags.m_canUseFastAccess = true;
        m_fastAccessIndex = index;
        m_fastAccessUpIndex = upIndex;
    }

    void unsetFastIndex()
    {
        m_flags.m_canUseFastAccess = false;
    }

    bool canUseFastAccess()
    {
        return m_flags.m_canUseFastAccess;
    }

    bool canUseFastAccessInHeap()
    {
        return m_flags.m_isFastAccessIndexIndicatesHeapIndex;
    }

    bool canUseGlobalFastAccess()
    {
        return m_flags.m_canUseGlobalFastAccess;
    }

    void dontUseFastAccess()
    {
        m_flags.m_canUseFastAccess = false;
    }

    bool onlySearchGlobal()
    {
        return m_flags.m_onlySearchGlobal;
    }

    bool fastAccessIndexIndicatesImmutableBinding()
    {
        return m_flags.m_fastAccessIndexIndicatesImmutableBinding;
    }

    void setFastAccessIndexImmutable(bool value)
    {
        m_flags.m_fastAccessIndexIndicatesImmutableBinding = value;
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
        m_flags.m_canUseGlobalFastAccess = true;
        m_fastAccessIndex = index;
    }

    size_t globalFastAccessIndex()
    {
        ASSERT(m_flags.m_canUseGlobalFastAccess);
        return m_fastAccessIndex;
    }

    void unsetGlobalFastIndex()
    {
        m_flags.m_canUseGlobalFastAccess = false;
    }

    virtual bool isIdentifier()
    {
        return true;
    }


protected:
    InternalAtomicString m_name;
    size_t m_fastAccessIndex;
    size_t m_fastAccessUpIndex;
    struct {
        bool m_canUseFastAccess:1;
        bool m_canUseGlobalFastAccess:1;
        bool m_isFastAccessIndexIndicatesHeapIndex:1;
        bool m_onlySearchGlobal:1;
        bool m_fastAccessIndexIndicatesImmutableBinding:1;
    } m_flags;
};

}

#endif
