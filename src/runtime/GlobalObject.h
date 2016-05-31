#ifndef GlobalObject_h
#define GlobalObject_h

#include "ESValue.h"

namespace escargot {

typedef ESErrorObject::Code ErrorCode;

#define RESOLVE_THIS_BINDING_TO_OBJECT(NAME, OBJ, BUILT_IN_METHOD) \
    ESValue thisVal = instance->currentExecutionContext()->resolveThisBinding(); \
    if (thisVal.isUndefinedOrNull()) { \
        throwBuiltinError(instance, ErrorCode::TypeError, strings->OBJ, true, strings->BUILT_IN_METHOD, errorMessage_GlobalObject_ThisUndefinedOrNull); \
    } \
    ESObject* NAME = thisVal.toObject();

#define RESOLVE_THIS_BINDING_TO_STRING(NAME, OBJ, BUILT_IN_METHOD) \
    ESValue thisVal = instance->currentExecutionContext()->resolveThisBinding(); \
    if (thisVal.isUndefinedOrNull()) { \
        throwBuiltinError(instance, ErrorCode::TypeError, strings->OBJ, true, strings->BUILT_IN_METHOD, errorMessage_GlobalObject_ThisUndefinedOrNull); \
    } \
    escargot::ESString* NAME = thisVal.toString();

NEVER_INLINE void throwBuiltinError(ESVMInstance* instance, ESErrorObject::Code code,
    const InternalAtomicString& objectName, bool prototoype, const InternalAtomicString& functionName, const char* templateString);

class JSBuiltinsObject;
class GlobalObject : public ESObject {
public:
    friend class ESVMInstance;
    GlobalObject();
    void finalize();

    ALWAYS_INLINE escargot::ESVMInstance* instance()
    {
        return m_instance;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* object()
    {
        return m_object;
    }

    ALWAYS_INLINE ESObject* objectPrototype()
    {
        return m_objectPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* error()
    {
        return m_error;
    }

    ALWAYS_INLINE ESObject* errorPrototype()
    {
        return m_errorPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* referenceError()
    {
        return m_referenceError;
    }

    ALWAYS_INLINE ESObject* referenceErrorPrototype()
    {
        return m_referenceErrorPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* typeError()
    {
        return m_typeError;
    }

    ALWAYS_INLINE ESObject* typeErrorPrototype()
    {
        return m_typeErrorPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* rangeError()
    {
        return m_rangeError;
    }

    ALWAYS_INLINE ESObject* rangeErrorPrototype()
    {
        return m_rangeErrorPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* syntaxError()
    {
        return m_syntaxError;
    }

    ALWAYS_INLINE ESObject* syntaxErrorPrototype()
    {
        return m_syntaxErrorPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* uriError()
    {
        return m_uriError;
    }

    ALWAYS_INLINE ESObject* uriErrorPrototype()
    {
        return m_uriErrorPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* evalError()
    {
        return m_evalError;
    }

    ALWAYS_INLINE ESObject* evalErrorPrototype()
    {
        return m_evalErrorPrototype;
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

    ALWAYS_INLINE escargot::ESObject* math()
    {
        return m_math;
    }

    ALWAYS_INLINE escargot::ESObject* json()
    {
        return m_json;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* number()
    {
        return m_number;
    }

    ALWAYS_INLINE escargot::ESObject* numberPrototype()
    {
        return m_numberPrototype;
    }

    ALWAYS_INLINE escargot::ESNumberObject* numberObjectProxy()
    {
        return m_numberObjectProxy;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* boolean()
    {
        return m_boolean;
    }

    ALWAYS_INLINE escargot::ESObject* booleanPrototype()
    {
        return m_booleanPrototype;
    }

    ALWAYS_INLINE escargot::ESBooleanObject* booleanObjectProxy()
    {
        return m_booleanObjectProxy;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* eval()
    {
        return m_eval;
    }

#ifdef USE_ES6_FEATURE
    ALWAYS_INLINE escargot::ESFunctionObject* int8Array()
    {
        return m_Int8Array;
    }

    ALWAYS_INLINE escargot::ESObject* int8ArrayPrototype()
    {
        return m_Int8ArrayPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* uint8Array()
    {
        return m_Uint8Array;
    }

    ALWAYS_INLINE escargot::ESObject* uint8ArrayPrototype()
    {
        return m_Uint8ArrayPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* int16Array()
    {
        return m_Int16Array;
    }

    ALWAYS_INLINE escargot::ESObject* int16ArrayPrototype()
    {
        return m_Int16ArrayPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* uint16Array()
    {
        return m_Uint16Array;
    }

    ALWAYS_INLINE escargot::ESObject* uint16ArrayPrototype()
    {
        return m_Uint16ArrayPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* int32Array()
    {
        return m_Int32Array;
    }

    ALWAYS_INLINE escargot::ESObject* int32ArrayPrototype()
    {
        return m_Int32ArrayPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* uint32Array()
    {
        return m_Uint32Array;
    }

    ALWAYS_INLINE escargot::ESObject* uint32ArrayPrototype()
    {
        return m_Uint32ArrayPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* uint8ClampedArray()
    {
        return m_Uint8ClampedArray;
    }

    ALWAYS_INLINE escargot::ESObject* uint8ClampedArrayPrototype()
    {
        return m_Uint8ClampedArrayPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* float32Array()
    {
        return m_Float32Array;
    }

    ALWAYS_INLINE escargot::ESObject* float32ArrayPrototype()
    {
        return m_Float32ArrayPrototype;
    }

    ALWAYS_INLINE escargot::ESFunctionObject* float64Array()
    {
        return m_Float64Array;
    }

    ALWAYS_INLINE escargot::ESObject* float64ArrayPrototype()
    {
        return m_Float64ArrayPrototype;
    }

    template <typename T>
    ALWAYS_INLINE escargot::ESObject* typedArrayPrototype()
    {
        switch (T::typeVal) {
        case TypedArrayType::Int8Array:
            return int8ArrayPrototype();
        case TypedArrayType::Uint8Array:
            return uint8ArrayPrototype();
        case TypedArrayType::Uint8ClampedArray:
            return uint8ClampedArrayPrototype();
        case TypedArrayType::Int16Array:
            return int16ArrayPrototype();
        case TypedArrayType::Uint16Array:
            return uint16ArrayPrototype();
        case TypedArrayType::Int32Array:
            return int32ArrayPrototype();
        case TypedArrayType::Uint32Array:
            return uint32ArrayPrototype();
        case TypedArrayType::Float32Array:
            return float32ArrayPrototype();
        case TypedArrayType::Float64Array:
            return float64ArrayPrototype();
        }
        RELEASE_ASSERT_NOT_REACHED();
    }

    ALWAYS_INLINE escargot::ESFunctionObject* arrayBuffer()
    {
        return m_arrayBuffer;
    }

    ALWAYS_INLINE escargot::ESObject* arrayBufferPrototype()
    {
        return m_arrayBufferPrototype;
    }
#endif

    // DO NOT USE THIS FUNCTION. THIS IS FOR GLOBAL OBJECT
    // NOTE rooted ESValue* has short life time.
    ALWAYS_INLINE ESBindingSlot addressOfProperty(escargot::ESString* key)
    {
        ASSERT(m_flags.m_isGlobalObject);
        ESObject* target = this;
        size_t ret;
        while (true) {
            ret = target->m_hiddenClass->findProperty(key);
            if (ret != SIZE_MAX)
                break;
            else
                if (target->__proto__().isESPointer() && target->__proto__().asESPointer()->isESObject())
                    target = target->__proto__().asESPointer()->asESObject();
                else
                    goto Empty;
        }
        if (target->m_hiddenClassData[ret].isDeleted())
            goto Empty;

        {
            ESHiddenClassPropertyInfo info = target->m_hiddenClass->propertyInfo(ret);
            return ESBindingSlot(&target->m_hiddenClassData[ret], info.isDataProperty(), info.isDataProperty() ? info.writable() : true, info.configurable(), true);
        }

        Empty:
        if (hasPropertyInterceptor()) {
            ESValue v = readKeyForPropertyInterceptor(key);
            if (!v.isDeleted())
                return new(GC) ESValue(v);
        }
        return nullptr;
    }

    ALWAYS_INLINE bool didSomePrototypeObjectDefineIndexedProperty()
    {
        return m_didSomePrototypeObjectDefineIndexedProperty;
    }
    void somePrototypeObjectDefineIndexedProperty();

    void propertyDeleted(size_t idx);
    void propertyDefined(size_t newIndex, escargot::ESString* name);

    void registerCodeBlock(CodeBlock* cb);
    void unregisterCodeBlock(CodeBlock* cb);

    void setIdentifierInterceptor(PropertyCallback cb)
    {
        m_identifierInterceptor = cb;
    }

    bool hasIdentifierInterceptor()
    {
        return m_identifierInterceptor;
    }

    ESValue readIdentifierFromIdentifierInterceptor(escargot::ESString* key)
    {
        ASSERT(m_identifierInterceptor);
        return m_identifierInterceptor(key, this);
    }

    PropertyCallback identifierInterceptor()
    {
        return m_identifierInterceptor;
    }

protected:
    void initGlobalObject();
    void installObject();
    void installFunction();
    void installError();
    void installArray();
    void installString();
    void installDate();
    void installMath();
    void installJSON();
    void installNumber();
    void installBoolean();
    void installRegExp();
#ifdef USE_ES6_FEATURE
    void installArrayBuffer();
    void installTypedArray();
    template <typename T>
    escargot::ESFunctionObject* installTypedArray(escargot::ESString*);
#endif

    escargot::ESVMInstance* m_instance;

    escargot::ESFunctionObject* m_object;
    escargot::ESObject* m_objectPrototype;
    escargot::ESFunctionObject* m_function;
    escargot::ESFunctionObject* m_functionPrototype;
    escargot::ESFunctionObject* m_array;
    escargot::ESArrayObject* m_arrayPrototype;
    escargot::ESFunctionObject* m_string;
    escargot::ESStringObject* m_stringPrototype;
    escargot::ESStringObject* m_stringObjectProxy;
    escargot::ESFunctionObject* m_date;
    escargot::ESDateObject* m_datePrototype;
    escargot::ESMathObject* m_math;
    escargot::ESObject* m_json;
    escargot::ESFunctionObject* m_number;
    escargot::ESNumberObject* m_numberPrototype;
    escargot::ESNumberObject* m_numberObjectProxy;
    escargot::ESFunctionObject* m_boolean;
    escargot::ESBooleanObject* m_booleanPrototype;
    escargot::ESBooleanObject* m_booleanObjectProxy;
    escargot::ESFunctionObject* m_regexp;
    escargot::ESRegExpObject* m_regexpPrototype;
    escargot::ESFunctionObject* m_error;
    escargot::ESErrorObject* m_errorPrototype;
    escargot::ESFunctionObject* m_referenceError;
    escargot::ESObject* m_referenceErrorPrototype;
    escargot::ESFunctionObject* m_typeError;
    escargot::ESObject* m_typeErrorPrototype;
    escargot::ESFunctionObject* m_rangeError;
    escargot::ESObject* m_rangeErrorPrototype;
    escargot::ESFunctionObject* m_syntaxError;
    escargot::ESObject* m_syntaxErrorPrototype;
    escargot::ESFunctionObject* m_uriError;
    escargot::ESObject* m_uriErrorPrototype;
    escargot::ESFunctionObject* m_evalError;
    escargot::ESObject* m_evalErrorPrototype;

#ifdef USE_ES6_FEATURE
    // Constructor and prototypes for TypedArray
    escargot::ESFunctionObject* m_Int8Array;
    escargot::ESObject* m_Int8ArrayPrototype;
    escargot::ESFunctionObject* m_Uint8Array;
    escargot::ESObject* m_Uint8ArrayPrototype;
    escargot::ESFunctionObject* m_Uint8ClampedArray;
    escargot::ESObject* m_Uint8ClampedArrayPrototype;
    escargot::ESFunctionObject* m_Int16Array;
    escargot::ESObject* m_Int16ArrayPrototype;
    escargot::ESFunctionObject* m_Uint16Array;
    escargot::ESObject* m_Uint16ArrayPrototype;
    escargot::ESFunctionObject* m_Int32Array;
    escargot::ESObject* m_Int32ArrayPrototype;
    escargot::ESFunctionObject* m_Uint32Array;
    escargot::ESObject* m_Uint32ArrayPrototype;
    escargot::ESFunctionObject* m_Float32Array;
    escargot::ESObject* m_Float32ArrayPrototype;
    escargot::ESFunctionObject* m_Float64Array;
    escargot::ESObject* m_Float64ArrayPrototype;
    escargot::ESFunctionObject* m_arrayBuffer;
    escargot::ESObject* m_arrayBufferPrototype;
#endif

    escargot::ESFunctionObject* m_eval;
    escargot::ESFunctionObject* m_objectProtoTypeToString;

    bool m_didSomePrototypeObjectDefineIndexedProperty;
    std::vector<CodeBlock*, pointer_free_allocator<CodeBlock*> > m_codeBlocks;

    PropertyCallback m_identifierInterceptor;
};

}

#endif
