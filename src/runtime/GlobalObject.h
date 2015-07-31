#ifndef GlobalObject_h
#define GlobalObject_h

#include "ESValue.h"

namespace escargot {

class JSBuiltinsObject;
class GlobalObject : public JSObject {
public:
    friend class ESVMInstance;
    GlobalObject();

    ALWAYS_INLINE escargot::JSFunction* object()
    {
        return m_object;
    }

    ALWAYS_INLINE JSObject* objectPrototype()
    {
        return m_objectPrototype;
    }

    ALWAYS_INLINE escargot::JSFunction* referenceError()
    {
        return m_referenceError;
    }

    ALWAYS_INLINE JSObject* referenceErrorPrototype()
    {
        return m_referenceErrorPrototype;
    }

    ALWAYS_INLINE escargot::JSFunction* function()
    {
        return m_function;
    }

    ALWAYS_INLINE escargot::JSFunction* functionPrototype()
    {
        return m_functionPrototype;
    }

    ALWAYS_INLINE escargot::JSFunction* array()
    {
        return m_array;
    }

    ALWAYS_INLINE JSObject* arrayPrototype()
    {
        return m_arrayPrototype;
    }

    ALWAYS_INLINE escargot::JSFunction* string()
    {
        return m_string;
    }

    ALWAYS_INLINE escargot::JSString* stringPrototype()
    {
        return m_stringPrototype;
    }

    ALWAYS_INLINE escargot::JSFunction* date()
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
    escargot::JSFunction* m_object;
    escargot::JSObject* m_objectPrototype;
    escargot::JSFunction* m_function;
    escargot::JSFunction* m_functionPrototype;
    escargot::JSFunction* m_referenceError;
    escargot::JSObject* m_referenceErrorPrototype;
    escargot::JSFunction* m_array;
    escargot::ESArrayObject* m_arrayPrototype;
    escargot::JSFunction* m_string;
    escargot::JSString* m_stringPrototype;
    escargot::JSFunction* m_date;
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
