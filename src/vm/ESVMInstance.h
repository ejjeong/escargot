#ifndef ESVMInstance_h
#define ESVMInstance_h

#include "runtime/GlobalObject.h"
#include "runtime/ESAtomicString.h"

namespace escargot {

class ExecutionContext;
class GlobalObject;

typedef std::unordered_map<std::wstring, ESAtomicStringData *,
        std::hash<std::wstring>,std::equal_to<std::wstring> > AtomicStringMap;

class ESVMInstance : public gc {
    friend class ESFunctionCaller;
public:
    ESVMInstance();
    void evaluate(const std::string& source);

    ALWAYS_INLINE ExecutionContext* currentExecutionContext() { return m_currentExecutionContext; }
    GlobalObject* globalObject() { return m_globalObject; }
protected:
    ExecutionContext* m_globalExecutionContext;
    ExecutionContext* m_currentExecutionContext;
    GlobalObject* m_globalObject;

    friend class ESAtomicString;
    friend class ESAtomicStringData;
    AtomicStringMap m_atomicStringMap;
};

}

#endif
