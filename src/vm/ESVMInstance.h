#ifndef ESVMInstance_h
#define ESVMInstance_h

namespace escargot {

class ExecutionContext;

class ESVMInstance : public gc_cleanup {
public:
    ESVMInstance();
    void evaluate(const std::string& source);

protected:
    ExecutionContext* m_globalExecutionContext;
    ExecutionContext* m_currentExecutionContext;
};

}

#endif
