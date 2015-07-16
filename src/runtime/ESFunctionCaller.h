#ifndef ESFunctionCaller_h
#define ESFunctionCaller_h

#include "ESValue.h"

namespace escargot {

class ESVMInstance;

class ESFunctionCaller {
public:
    static ESValue* call(ESValue* callee, ESValue* receiver, ESValue* arguments[], size_t argumentCount, ESVMInstance* ESVMInstance);
};

}

#endif
