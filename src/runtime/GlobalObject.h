#ifndef GlobalObject_h
#define GlobalObject_h
#include "ESValue.h"
#include "ESValueInlines.h"

namespace escargot {

class JSBuiltinsObject;
class GlobalObject : public JSObject {
    JSBuiltinsObject* m_builtins;
    Context* m_nativeContext;
};

class JSGlobalObject : public GlobalObject {
};

class JSBuiltinsObject : public GlobalObject {
};

}

#endif
