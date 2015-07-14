#ifndef ExecutionContext_h
#define ExecutionContext_h

#include "ESValue.h"
#include "ESValueInlines.h"

namespace escargot {

class ESVMInstance;

class ESFunctionCaller {
    static ESValue call(ESValue callee, ESValue receiver, ESValue* arguments, size_t argumentCount, ESVMInstance* ESVMInstance);
};

}

#endif
