#ifndef ESVMInstance_h
#define ESVMInstance_h

#include "runtime/GlobalObject.h"
#include "runtime/ESAtomicString.h"

namespace escargot {

class ExecutionContext;
class GlobalObject;
class ESVMInstance;

extern __thread ESVMInstance* currentInstance;

typedef std::unordered_map<std::wstring, ESAtomicStringData *,
        std::hash<std::wstring>,std::equal_to<std::wstring> > AtomicStringMap;

class ESVMInstance : public gc {
    friend class ESFunctionCaller;
public:
    ESVMInstance();
    void evaluate(const std::string& source);

    ALWAYS_INLINE ExecutionContext* currentExecutionContext() { return m_currentExecutionContext; }
    GlobalObject* globalObject() { return m_globalObject; }

    void enter();
    void exit();
    static ESVMInstance* currentInstance()
    {
        return escargot::currentInstance;
    }

    ALWAYS_INLINE Strings& strings() { return m_strings; }
protected:
    ExecutionContext* m_globalExecutionContext;
    ExecutionContext* m_currentExecutionContext;
    GlobalObject* m_globalObject;

    friend class ESAtomicString;
    friend class ESAtomicStringData;
    AtomicStringMap m_atomicStringMap;

    Strings m_strings;
};

}

#endif
