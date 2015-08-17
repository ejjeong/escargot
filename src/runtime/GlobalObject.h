#ifndef GlobalObject_h
#define GlobalObject_h

#include "ESValue.h"

namespace escargot {

class JSBuiltinsObject;
class GlobalObject : public ESObject {
public:
    friend class ESVMInstance;
    GlobalObject();

    ALWAYS_INLINE escargot::ESFunctionObject* object()
    {
        return m_object;
    }

    ALWAYS_INLINE ESObject* objectPrototype()
    {
        return m_objectPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* referenceError()
    {
        return m_referenceError;
    }

    ALWAYS_INLINE ESObject* referenceErrorPrototype()
    {
        return m_referenceErrorPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* regexp()
    {
        return m_regexp;
    }

    ALWAYS_INLINE escargot::ESRegExpObject* regexpPrototype()
    {
        return m_regexpPrototype;
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

    ALWAYS_INLINE ESObject* arrayPrototype()
    {
        return m_arrayPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* string()
    {
        return m_string;
    }

    ALWAYS_INLINE escargot::ESStringObject* stringPrototype()
    {
        return m_stringPrototype;
    }

    ALWAYS_INLINE escargot::ESStringObject* stringObjectProxy()
    {
        return m_stringObjectProxy;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* date()
    {
        return m_date;
    }

    ALWAYS_INLINE escargot::ESDateObject* datePrototype()
    {
        return m_datePrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* math()
    {
        return m_math;
    }

    ALWAYS_INLINE escargot::ESObject* mathPrototype()
    {
        return m_mathPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* number()
    {
        return m_number;
    }

    ALWAYS_INLINE escargot::ESObject* numberPrototype()
    {
        return m_numberPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* boolean()
    {
        return m_boolean;
    }

    ALWAYS_INLINE escargot::ESObject* booleanPrototype()
    {
        return m_booleanPrototype;
    }

protected:
    void installObject();
    void installFunction();
    void installError();
    void installArray();
    void installString();
    void installDate();
    void installMath();
    void installNumber();
    void installBoolean();
    void installRegExp();
    escargot::ESFunctionObject* m_object;
    escargot::ESObject* m_objectPrototype;
    escargot::ESFunctionObject* m_function;
    escargot::ESFunctionObject* m_functionPrototype;
    escargot::ESObject* m_referenceErrorPrototype;
    escargot::ESFunctionObject* m_array;
    escargot::ESArrayObject* m_arrayPrototype;
    escargot::ESFunctionObject* m_string;
    escargot::ESStringObject* m_stringPrototype;
    escargot::ESStringObject* m_stringObjectProxy;
    escargot::ESFunctionObject* m_date;
    escargot::ESDateObject* m_datePrototype;
    escargot::ESFunctionObject* m_math;
    escargot::ESObject* m_mathPrototype;
    escargot::ESFunctionObject* m_number;
    escargot::ESNumberObject* m_numberPrototype;
    escargot::ESFunctionObject* m_boolean;
    escargot::ESBooleanObject* m_booleanPrototype;
    escargot::ESFunctionObject* m_regexp;
    escargot::ESRegExpObject* m_regexpPrototype;
    escargot::ESFunctionObject* m_error;
    escargot::ESObject* m_errorPrototype;
    escargot::ESFunctionObject* m_referenceError;
    //JSBuiltinsObject* m_builtins;
    //Context* m_nativeContext;
};

class JSGlobalObject : public GlobalObject {
};

class JSBuiltinsObject : public GlobalObject {
};

}

#endif
