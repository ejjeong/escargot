#ifndef ESVMInstance_h
#define ESVMInstance_h

#include "runtime/GlobalObject.h"

namespace escargot {

class ExecutionContext;
class GlobalObject;

class ESVMInstance : public gc_cleanup {
public:
    ESVMInstance();
    void evaluate(const std::string& source);

    ALWAYS_INLINE ExecutionContext* currentExecutionContext() { return m_currentExecutionContext; }
protected:
    ExecutionContext* m_globalExecutionContext;
    ExecutionContext* m_currentExecutionContext;
    GlobalObject* m_globalObject;
};

}

#endif
