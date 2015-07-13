#ifndef ESVMInstance_h
#define ESVMInstance_h

#include <string>
class ExecutionContext;
class ESVMInstance {
public:
    ESVMInstance();

    void evaluate(std::string source);
    ExecutionContext* globalExecutionCtx;
    ExecutionContext* currentExecutionCtx;
};

#endif
