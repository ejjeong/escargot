#ifndef FunctionNode_h
#define FunctionNode_h

#include "Node.h"
#include "PatternNode.h"
#include "ExpressionNode.h"


namespace escargot {

class FunctionNode : public Node {
public:
    friend ESValue ESFunctionObject::call(ESVMInstance* instance, const ESValue& callee, const ESValue& receiver, ESValue arguments[], const size_t& argumentCount, bool isNewExpression);
    FunctionNode(NodeType type , const InternalAtomicString& id, InternalAtomicStringVector&& params,
        Node* body, bool isGenerator, bool isExpression, bool isStrict)
            : Node(type)
    {
        m_id = id;
        m_nonAtomicId = id.string();
        m_params = params;
        m_body = body;
        m_isGenerator = isGenerator;
        m_isExpression = isExpression;
        m_needsActivation = false;
        m_needsHeapAllocatedExecutionContext = false;
        m_needsToPrepareGenerateArgumentsObject = false;
        m_needsComplexParameterCopy = false;
        m_outerFunctionNode = NULL;
        m_isStrict = isStrict;
        m_isExpression = false;
        m_functionIdIndex = -1;
        m_functionIdIndexNeedsHeapAllocation = true;
    }

    ALWAYS_INLINE const InternalAtomicStringVector& params() { return m_params; }
    ALWAYS_INLINE FunctionParametersInfoVector& paramsInformation() { return m_paramsInformation; }
    ALWAYS_INLINE Node* body() { return m_body; }
    ALWAYS_INLINE const InternalAtomicString& id() { return m_id; }
    ALWAYS_INLINE ESString* nonAtomicId() { return m_nonAtomicId; }
    ALWAYS_INLINE size_t stackAllocatedIdentifiersCount() { return m_stackAllocatedIdentifiersCount; }
    ALWAYS_INLINE bool needsActivation() { return m_needsActivation; } // child & parent AST has eval, with
    ALWAYS_INLINE void setNeedsActivation()
    {
        m_needsActivation = true;
        setNeedsHeapAllocatedExecutionContext();
    }
    ALWAYS_INLINE bool needsHeapAllocatedExecutionContext() { return m_needsHeapAllocatedExecutionContext; }
    ALWAYS_INLINE void setNeedsHeapAllocatedExecutionContext() { m_needsHeapAllocatedExecutionContext = true; }
    ALWAYS_INLINE bool needsToPrepareGenerateArgumentsObject() { return m_needsToPrepareGenerateArgumentsObject; }
    ALWAYS_INLINE void setNeedsToPrepareGenerateArgumentsObject() { m_needsToPrepareGenerateArgumentsObject = true; }
    ALWAYS_INLINE bool needsComplexParameterCopy() { return m_needsComplexParameterCopy; }
    ALWAYS_INLINE bool isGenerator() { return m_isGenerator; }
    ALWAYS_INLINE bool isExpression() { return m_isExpression; }
    ALWAYS_INLINE bool isStrict() { return m_isStrict; }
    ALWAYS_INLINE unsigned argumentCount() { return m_params.size(); }


    void setInnerIdentifierInfo(InnerIdentifierInfoVector&& vec)
    {
        m_innerIdentifiers = vec;
    }

    InnerIdentifierInfoVector& innerIdentifiers() { return m_innerIdentifiers; }
    InternalAtomicStringVector& heapAllocatedIdentifiers() { return m_heapAllocatedIdentifiers; }

    void setOuterFunctionNode(FunctionNode* o) { m_outerFunctionNode = o; }
    FunctionNode* outerFunctionNode() { return m_outerFunctionNode; }

    void generateInformationForCodeBlock()
    {
        size_t siz = m_params.size();
        m_paramsInformation.resize(siz);
        size_t heapCount = 0;
        size_t stackCount = 0;
        for (size_t i = 0; i < siz; i ++) {
            bool isHeap = m_innerIdentifiers[i].m_flags.m_isHeapAllocated;
            m_paramsInformation[i].m_isHeapAllocated = isHeap;
            if (isHeap) {
                m_paramsInformation[i].m_index = i - stackCount;
                m_needsComplexParameterCopy = true;
                heapCount++;
            } else {
                m_paramsInformation[i].m_index = i - heapCount;
                stackCount++;
            }
        }

        m_stackAllocatedIdentifiersCount = 0;
        siz = m_innerIdentifiers.size();
        for (size_t i = 0; i < siz; i ++) {
            if (m_innerIdentifiers[i].m_flags.m_isHeapAllocated) {
                m_heapAllocatedIdentifiers.push_back(m_innerIdentifiers[i].m_name);
            } else {
                m_stackAllocatedIdentifiersCount++;
            }
        }
    }
protected:
    InternalAtomicString m_id; // id: Identifier;
    ESString* m_nonAtomicId; // id: Identifier;
    InternalAtomicStringVector m_params; // params: [ Pattern ];
    InnerIdentifierInfoVector m_innerIdentifiers;

    size_t m_stackAllocatedIdentifiersCount;
    InternalAtomicStringVector m_heapAllocatedIdentifiers;
    FunctionParametersInfoVector m_paramsInformation;
    // defaults: [ Expression ];
    // rest: Identifier | null;
    Node* m_body; // body: BlockStatement | Expression;
    bool m_isGenerator; // generator: boolean;
    bool m_isExpression; // expression: boolean;

    bool m_needsActivation; // child & parent AST has eval, with
    bool m_needsHeapAllocatedExecutionContext;
    bool m_needsComplexParameterCopy; // parameters are captured
    bool m_needsToPrepareGenerateArgumentsObject;
    FunctionNode* m_outerFunctionNode;

    bool m_isStrict;

    bool m_functionIdIndexNeedsHeapAllocation;
    size_t m_functionIdIndex;
};

}

#endif
