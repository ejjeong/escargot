#include "ESVMInstance.h"
#include "Environment.h"
#include "ExecutionContext.h"

ESVMInstance::ESVMInstance() {
    auto a = new LexicalEnvironment();
    a->record = new GlobalEnvironmentRecord();
    a->outerEnv = NULL;
    
    globalExecutionCtx = new ExecutionContext(a, a);
    currentExecutionCtx = globalExecutionCtx;
}

void ESVMInstance::evaluate(std::string source) {
}
