#ifndef GlobalObject_h
#define GlobalObject_h

#include "ESValue.h"

namespace escargot {

class JSBuiltinsObject;
class GlobalObject : public JSObject {
public:
    friend class ESVMInstance;
    GlobalObject();

    ALWAYS_INLINE escargot::ESFunctionObject* object()
    {
        return m_object;
    }

    ALWAYS_INLINE JSObject* objectPrototype()
    {
        return m_objectPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* referenceError()
    {
        return m_referenceError;
    }

    ALWAYS_INLINE JSObject* referenceErrorPrototype()
    {
        return m_referenceErrorPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* function()
    {
        return m_function;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* functionPrototype()
    {
        return m_functionPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* array()
    {
        return m_array;
    }

    ALWAYS_INLINE JSObject* arrayPrototype()
    {
        return m_arrayPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* string()
    {
        return m_string;
    }

    ALWAYS_INLINE escargot::JSString* stringPrototype()
    {
        return m_stringPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* date()
    {
        return m_date;
    }

    ALWAYS_INLINE escargot::ESDateObject* datePrototype()
    {
        return m_datePrototype;
    }

protected:
    void installObject();
    void installFunction();
    void installError();
    void installArray();
    void installString();
    void installDate();
    escargot::ESFunctionObject* m_object;
    escargot::JSObject* m_objectPrototype;
    escargot::ESFunctionObject* m_function;
    escargot::ESFunctionObject* m_functionPrototype;
    escargot::ESFunctionObject* m_referenceError;
    escargot::JSObject* m_referenceErrorPrototype;
    escargot::ESFunctionObject* m_array;
    escargot::ESArrayObject* m_arrayPrototype;
    escargot::ESFunctionObject* m_string;
    escargot::JSString* m_stringPrototype;
    escargot::ESFunctionObject* m_date;
    escargot::ESDateObject* m_datePrototype;
    //JSBuiltinsObject* m_builtins;
    //Context* m_nativeContext;
};

class JSGlobalObject : public GlobalObject {
};

class JSBuiltinsObject : public GlobalObject {
};

}

#endif
