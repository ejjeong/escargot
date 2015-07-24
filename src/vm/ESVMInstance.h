#ifndef ESVMInstance_h
#define ESVMInstance_h

#include "runtime/GlobalObject.h"

namespace escargot {

class ExecutionContext;
class GlobalObject;

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
};

}

#endif
