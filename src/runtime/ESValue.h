#ifndef ESValue_h
#define ESValue_h
#include "ESString.h"

namespace escargot {

class Smi;
class HeapObject;

class ESValue {
    //static void* operator new(size_t, void* p) = delete;
    //static void* operator new[](size_t, void* p) = delete;
    //static void* operator new(size_t size) = delete;
    //static void* operator new[](size_t size) = delete;
protected:
    ESValue() { }
public:

    bool isSmi() const;
    bool isHeapObject() const;
    Smi* toSmi() const;
    HeapObject* toHeapObject() const;
};

class Smi : public ESValue {
public:
    int value();
    static inline Smi* fromInt(int value);
    static inline Smi* fromIntptr(intptr_t value);
};


class Undefined;
class Null;
class Boolean;
class Number;
class String;
class JSObject;
class JSObjectSlot;
class JSArray;
class JSFunction;
class FunctionDeclarationNode;

class HeapObject : public ESValue, public gc_cleanup {
public:
    enum Type {
        Undefined = 1,
        Null = 1 << 1,
        Boolean = 1 << 2,
        Number = 1 << 3,
        String = 1 << 4,
        JSObject = 1 << 5,
        JSObjectSlot = 1 << 6,
        JSFunction = 1 << 7,
        JSArray = 1 << 8,
        TypeMask = 0xff
    };

protected:
    HeapObject(Type type)
    {
        m_data = m_data | type;
    }

public:

    ALWAYS_INLINE Type type()
    {
        return (Type)(m_data & TypeMask);
    }

    ALWAYS_INLINE bool isUndefined()
    {
        return m_data & Type::Undefined;
    }

    ALWAYS_INLINE ::escargot::Undefined* toUndefined()
    {
#ifndef NDEBUG
        ASSERT(isUndefined());
#endif
        return reinterpret_cast<::escargot::Undefined *>(this);
    }

    ALWAYS_INLINE bool isNull()
    {
        return m_data & Type::Null;
    }

    ALWAYS_INLINE ::escargot::Null* toNull()
    {
#ifndef NDEBUG
        ASSERT(isNull());
#endif
        return reinterpret_cast<::escargot::Null *>(this);
    }

    ALWAYS_INLINE bool isBoolean()
    {
        return m_data & Type::Boolean;
    }

    ALWAYS_INLINE ::escargot::Boolean* toBoolean()
    {
#ifndef NDEBUG
        ASSERT(isBoolean());
#endif
        return reinterpret_cast<::escargot::Boolean *>(this);
    }

    ALWAYS_INLINE bool isNumber()
    {
        return m_data & Type::Number;
    }

    ALWAYS_INLINE ::escargot::Number* toNumber()
    {
#ifndef NDEBUG
        ASSERT(isNumber());
#endif
        return reinterpret_cast<::escargot::Number*>(this);
    }

    ALWAYS_INLINE bool isString()
    {
        return m_data & Type::String;
    }

    ALWAYS_INLINE ::escargot::String* toString()
    {
#ifndef NDEBUG
        ASSERT(isString());
#endif
        return reinterpret_cast<::escargot::String *>(this);
    }

    ALWAYS_INLINE bool isJSObject()
    {
        return m_data & Type::JSObject;
    }

    ALWAYS_INLINE ::escargot::JSObject* toJSObject()
    {
#ifndef NDEBUG
        ASSERT(isJSObject());
#endif
        return reinterpret_cast<::escargot::JSObject *>(this);
    }

    ALWAYS_INLINE bool isJSObjectSlot()
    {
        return m_data & Type::JSObjectSlot;
    }

    ALWAYS_INLINE ::escargot::JSObjectSlot* toJSObjectSlot()
    {
#ifndef NDEBUG
        ASSERT(isJSObjectSlot());
#endif
        return reinterpret_cast<::escargot::JSObjectSlot *>(this);
    }

    ALWAYS_INLINE bool isJSArray()
    {
        return m_data & Type::JSArray;
    }

    ALWAYS_INLINE ::escargot::JSArray* toJSArray()
    {
#ifndef NDEBUG
        ASSERT(isJSArray());
#endif
        return reinterpret_cast<::escargot::JSArray *>(this);
    }

    ALWAYS_INLINE bool isJSFunction()
    {
        return m_data & Type::JSFunction;
    }

    ALWAYS_INLINE ::escargot::JSFunction* toJSFunction()
    {
#ifndef NDEBUG
        ASSERT(isJSFunction());
#endif
        return reinterpret_cast<::escargot::JSFunction *>(this);
    }

protected:
    // 0x******@@
    // * -> Data
    // @ -> tag
    int m_data;
};

class Undefined : public HeapObject {
protected:
public:
    Undefined()
        : HeapObject(HeapObject::Type::Undefined)
    {

    }
    static Undefined* create()
    {
        return new Undefined();
    }
};

extern Undefined* undefined;

class Null : public HeapObject {
protected:
public:
    Null()
        : HeapObject(HeapObject::Type::Null)
    {

    }
    static Null* create()
    {
        return new Null();
    }
};

extern Null* null;


class Boolean : public HeapObject {
protected:
    const int DataMask = 0x80000000;
    Boolean(bool b)
        : HeapObject(HeapObject::Type::Boolean)
    {
        set(b);
    }
public:
    static Boolean* create(bool b)
    {
        return new Boolean(b);
    }

    ALWAYS_INLINE void set(bool b)
    {
        m_data = (m_data & TypeMask) | (b << 31);
    }

    ALWAYS_INLINE bool get()
    {
        return m_data & DataMask;
    }
};

class Number : public HeapObject {
    Number(double value)
        : HeapObject(HeapObject::Type::Number)
    {
        set(value);
    }
public:
    static Number* create(double value)
    {
        return new Number(value);
    }

    ALWAYS_INLINE void set(double b)
    {
        m_value = b;
    }

    ALWAYS_INLINE double get()
    {
        return m_value;
    }
protected:
    double m_value;
};

class String : public HeapObject {
protected:
    String(const ESString& src)
        : HeapObject(HeapObject::Type::String)
    {
        m_string = src;
    }
public:
    static String* create(const ESString& src)
    {
        return new String(src);
    }

    const ESString& string()
    {
        return m_string;
    }

protected:
    ESString m_string;
};

class JSObjectSlot : public HeapObject {
    JSObjectSlot(ESValue* value,bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true)
        : HeapObject(HeapObject::Type::JSObjectSlot)
    {
        m_value = value;
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
    }
public:
    static JSObjectSlot* create(ESValue* value,bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true)
    {
        return new JSObjectSlot(value, isWritable, isEnumerable, isConfigurable);
    }

    void setValue(ESValue* value)
    {
        m_value = value;
    }

    ESValue* value()
    {
        return m_value;
    }

    bool isConfigurable()
    {
        return m_isConfigurable;
    }

protected:
    ESValue* m_value;
    bool m_isWritable:1;
    bool m_isEnumerable:1;
    bool m_isConfigurable:1;
};

class JSObject : public HeapObject {
protected:
    JSObject(HeapObject::Type type = HeapObject::Type::JSObject)
        : HeapObject(type)
    {
    }
public:
    void definePropertyOrThrow(const ESString& key, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true)
    {
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            escargot::JSObjectSlot* v = escargot::JSObjectSlot::create(undefined, isWritable, isEnumerable, isConfigurable);
            m_map.insert(std::make_pair(key, v));
        } else {
            //TODO
        }
    }

    bool hasOwnProperty(const ESString& key) {
        auto iter = m_map.find(key);
        if(iter == m_map.end())
            return false;
        return true;
    }

    //$6.1.7.2 Object Internal Methods and Internal Slots
    bool isExtensible() {
        return true;
    }
    /*
    PropertyDescriptor getOwnProperty(const ESString& ekey) {
        std::wstring key = std::wstring(ESString(ekey).data());
        std::unordered_map<std::wstring, PropertyDescriptor>::iterator it = m_map.find(key);
        if(it != m_map.end())
            return it->second;
        //FIXME
        PropertyDescriptor pd_undefined;
        return pd_undefined;
    }*/

    static JSObject* create()
    {
        return new JSObject();
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-o-p
    ESValue* get(const ESString& key)
    {
        //TODO Assert: IsPropertyKey(P) is true.
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            return undefined;
        }
        return iter->second->value();
    }

    escargot::JSObjectSlot* find(const ESString& key)
    {
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            return NULL;
        }
        return iter->second;
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
    void set(const ESString& key, ESValue* val, bool shouldThrowException = false)
    {
        //TODO Assert: IsPropertyKey(P) is true.
        //TODO Assert: Type(Throw) is Boolean.
        //TODO shouldThrowException
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            escargot::JSObjectSlot* v = escargot::JSObjectSlot::create(val);
            m_map.insert(std::make_pair(key, v));
        } else {
            iter->second->setValue(val);
        }

    }

    bool hasKey(const ESString& key)
    {
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            return false;
        }
        return true;
    }

protected:
    std::unordered_map<ESString, escargot::JSObjectSlot *,
            std::hash<ESString>,std::equal_to<ESString>,
            gc_allocator<std::pair<const ESString, escargot::JSObjectSlot *> > > m_map;
};

class JSArray : public JSObject {
};

class LexicalEnvironment;
class Node;
class JSFunction : public JSObject {
protected:
    JSFunction(LexicalEnvironment* outerEnvironment, FunctionDeclarationNode* functionAST)
        : JSObject((Type)(Type::JSObject | Type::JSFunction))
    {
        m_outerEnvironment = outerEnvironment;
        m_functionAST = functionAST;
    }
public:
    static JSFunction* create(LexicalEnvironment* outerEnvironment, FunctionDeclarationNode* functionAST)
    {
        return new JSFunction(outerEnvironment, functionAST);
    }

    FunctionDeclarationNode* functionAST() { return m_functionAST; }
    LexicalEnvironment* outerEnvironment() { return m_outerEnvironment; }
protected:
    LexicalEnvironment* m_outerEnvironment;
    FunctionDeclarationNode* m_functionAST;
    enum ThisBindingStatus {
        lexical, initialized, uninitialized
    };
    ThisBindingStatus m_thisBindingStatus;
    //JSObject functionObject;
    //HomeObject
    ////JSObject newTarget
    //BindThisValue(V);
    //GetThisBinding();
};

}

#include "ESValueInlines.h"

#endif
