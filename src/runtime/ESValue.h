#ifndef ESValue_h
#define ESValue_h
#include "ESString.h"

namespace escargot {

class Smi;
class HeapObject;
class Undefined;
class Null;
class Boolean;
class Number;
class String;
class JSObject;
class JSSlot;
class JSArray;
class JSFunction;
class FunctionNode;
class ESVMInstance;

extern Undefined* esUndefined;
extern Null* esNull;
extern Boolean* esTrue;
extern Boolean* esFalse;

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
    ESString toESString();
    ESValue* toPrimitive();
    ESValue* toNumber();
    ESValue* toInt32();
    ALWAYS_INLINE ESValue* ensureValue();
};

class Smi : public ESValue {
public:
    int value();
    static inline Smi* fromInt(int value);
    static inline Smi* fromIntptr(intptr_t value);
};

class HeapObject : public ESValue, public gc {
public:
    enum Type {
        Undefined = 1,
        Null = 1 << 1,
        Boolean = 1 << 2,
        Number = 1 << 3,
        String = 1 << 4,
        JSObject = 1 << 5,
        JSSlot = 1 << 6,
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

    ALWAYS_INLINE bool isJSSlot()
    {
        return m_data & Type::JSSlot;
    }

    ALWAYS_INLINE ::escargot::JSSlot* toJSSlot()
    {
#ifndef NDEBUG
        ASSERT(isJSSlot());
#endif
        return reinterpret_cast<::escargot::JSSlot *>(this);
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


class Boolean : public HeapObject {
protected:
    const int DataMask = 0x80000000;
public:
    Boolean(bool b)
        : HeapObject(HeapObject::Type::Boolean)
    {
        set(b);
    }
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


class JSSlot : public HeapObject {
    JSSlot(::escargot::ESValue* value,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
        : HeapObject(Type::JSSlot)
    {
        m_data.m_value = value;
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = true;
    }

    JSSlot(::escargot::JSObject* object,
            std::function<ESValue* (::escargot::JSObject* obj)> getter = nullptr,
            std::function<void (::escargot::JSObject* obj, ESValue* value)> setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
        : HeapObject(Type::JSSlot)
    {
        m_data.m_object = object;
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = false;
        m_getter = getter;
        m_setter = setter;
    }

public:
    static JSSlot* create(::escargot::ESValue* value,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        return new JSSlot(value, isWritable, isEnumerable, isConfigurable);
    }

    static JSSlot* create(::escargot::JSObject* object,
            std::function<ESValue* (::escargot::JSObject* obj)> getter = nullptr,
            std::function<void (::escargot::JSObject* obj, ESValue* value)> setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        return new JSSlot(object, getter, setter, isWritable, isEnumerable, isConfigurable);
    }

    void setValue(ESValue* value)
    {
        if(m_isDataProperty) {
            m_data.m_value = value;
        } else {
            if(m_setter) {
                m_setter(m_data.m_object, value);
            }
        }

    }

    ESValue* value()
    {
        if(m_isDataProperty) {
            return m_data.m_value;
        } else {
            if(m_getter) {
                return m_getter(m_data.m_object);
            }
            return esUndefined;
        }
    }

    bool isConfigurable()
    {
        return m_isConfigurable;
    }

    bool isEnumerable()
    {
        return m_isEnumerable;
    }

    bool isWritable()
    {
        return m_isWritable;
    }

protected:
    union {
        ESValue* m_value;
        ::escargot::JSObject* m_object;
    } m_data;

    std::function<ESValue* (::escargot::JSObject* obj)> m_getter;
    std::function<void (::escargot::JSObject* obj, ESValue* value)> m_setter;
    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-property-attributes
    bool m_isDataProperty:1;
    bool m_isWritable:1;
    bool m_isEnumerable:1;
    bool m_isConfigurable:1;
};


typedef std::unordered_map<ESAtomicString, ::escargot::JSSlot *,
                std::hash<ESAtomicString>,std::equal_to<ESAtomicString>,
                gc_allocator<std::pair<const ESAtomicString, ::escargot::JSSlot *> > > JSObjectMapStd;

/*
typedef std::map<ESString, ::escargot::JSSlot *,
            std::less<ESString>,
            gc_allocator<std::pair<const ESString, ::escargot::JSSlot *> > > JSObjectMapStd;
*/
class JSObjectMap : public JSObjectMapStd {
public:
    JSObjectMap(size_t siz)
        : JSObjectMapStd(siz) { }

};

class JSObject : public HeapObject {
    friend class JSSlot;
protected:
    JSObject(HeapObject::Type type = HeapObject::Type::JSObject)
        : HeapObject(type)
        , m_map(16)
    {
        m___proto__ = esNull;

        //FIXME set proper flags(is...)
        definePropertyOrThrow(strings->constructor, true, false, false);

        defineAccessorProperty(strings->__proto__, [](JSObject* self) -> ESValue* {
            return self->__proto__();
        },[](::escargot::JSObject* self, ESValue* value){
            if(value->isHeapObject() && value->toHeapObject()->isJSObject())
                self->set__proto__(value->toHeapObject()->toJSObject());
        }, true, false, false);
    }
public:
    void definePropertyOrThrow(const ESAtomicString& key, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true)
    {
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            m_map.insert(std::make_pair(key, JSSlot::create(esUndefined, isWritable, isEnumerable, isConfigurable)));
        } else {
            //TODO
        }
    }

    bool hasOwnProperty(const ESAtomicString& key) {
        auto iter = m_map.find(key);
        if(iter == m_map.end())
            return false;
        return true;
    }

    //$6.1.7.2 Object Internal Methods and Internal Slots
    bool isExtensible() {
        return true;
    }

    static JSObject* create()
    {
        return new JSObject();
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-o-p
    ESValue* get(const ESAtomicString& key)
    {
        ESValue* ret = esUndefined;
        //TODO Assert: IsPropertyKey(P) is true.
        auto iter = m_map.find(key);
        if(iter != m_map.end()) {
            ret = iter->second->value();
        }
        return ret;
    }

    ALWAYS_INLINE escargot::JSSlot* find(const ESAtomicString& key)
    {
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            return NULL;
        }
        return iter->second;
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
    void set(const ESAtomicString& key, ESValue* val, bool shouldThrowException = false)
    {
        //TODO Assert: IsPropertyKey(P) is true.
        //TODO Assert: Type(Throw) is Boolean.
        //TODO shouldThrowException
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            //TODO set flags
            m_map.insert(std::make_pair(key, escargot::JSSlot::create(val, true, true, true)));
        } else {
            iter->second->setValue(val);
        }
    }

    void set(ESValue* key, ESValue* val, bool shouldThrowException = false)
    {
        set(ESAtomicString(key->toESString().data()), val, shouldThrowException);
    }

    void defineAccessorProperty(const ESAtomicString& key,std::function<ESValue* (::escargot::JSObject* obj)> getter = nullptr,
            std::function<void (::escargot::JSObject* obj, ESValue* value)> setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        auto iter = m_map.find(key);
        if(iter != m_map.end()) {
            m_map.erase(iter);
        }
        m_map.insert(std::make_pair(key, escargot::JSSlot::create(this, getter, setter, isWritable, isEnumerable, isConfigurable)));

    }

    bool hasKey(const ESAtomicString& key)
    {
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            return false;
        }
        return true;
    }

    //FUNCTION FOR DEBUG.
    template <typename Functor>
    void enumeration(Functor t)
    {
        auto iter = m_map.begin();
        while(iter != m_map.end()) {
            if(iter->second->isEnumerable()) {
                t((*iter).first,(*iter).second);
            }
            iter++;
        }
    }

    ALWAYS_INLINE ESValue* __proto__()
    {
        return m___proto__;
    }

    ALWAYS_INLINE void set__proto__(ESValue* obj)
    {
        m___proto__ = obj;
    }

    ALWAYS_INLINE ESValue* constructor()
    {
        return get(strings->constructor);
    }

    ALWAYS_INLINE void setConstructor(ESValue* obj)
    {
        set(strings->constructor, obj);
    }

protected:
    JSObjectMap m_map;
    ESValue* m___proto__;
};

class JSArray : public JSObject {
protected:
    JSArray(HeapObject::Type type = HeapObject::Type::JSArray)
        : JSObject((Type)(Type::JSObject | Type::JSArray))
    {
        m_length = Smi::fromInt(0);
    }
public:
    static JSArray* create()
    {
        return JSArray::create(0);
    }

    // $9.4.2.2
    static JSArray* create(int length, JSObject* proto = NULL)
    {
        //TODO
        JSArray* arr = new JSArray();
        arr->setLength(length);
        /*
        if(proto == NULL)
            proto = global->arrayPrototype();
        setPrototype(proto);
        */
        return arr;
    }

    void set(const ESAtomicString& key, ESValue* val, bool shouldThrowException = false)
    {
        if (key == strings->length)
            setLength(val);
        else {
            JSObject::set(key, val, shouldThrowException);
        }
    }

    void set(ESValue* key, ESValue* val, bool shouldThrowException = false) {
        if (key->isSmi()) {
            int i = key->toSmi()->value()+1;
            if (i > length()->toSmi()->value())
                setLength(i);
        }
        set(ESAtomicString(key->toESString().data()), val, shouldThrowException);
    }

    void setLength(ESValue* len)
    {
        ASSERT(len->isSmi());
        JSObject::set(strings->length, len, false);
        if (len->toSmi() < m_length->toSmi()) {
            //TODO : delete elements
        }
        m_length = len;
    }

    void setLength(int len)
    {
        auto length = Smi::fromInt(len);
        JSObject::set(strings->length, length, false);
        m_length = length;
    }

    ESValue* length()
    {
        return m_length;
    }

protected:
    ESValue* m_length;
};

class LexicalEnvironment;
class Node;
class JSFunction : public JSObject {
protected:
    JSFunction(LexicalEnvironment* outerEnvironment, FunctionNode* functionAST)
        : JSObject((Type)(Type::JSObject | Type::JSFunction))
    {
        m_outerEnvironment = outerEnvironment;
        m_functionAST = functionAST;
        m_protoType = esUndefined;

        defineAccessorProperty(strings->prototype, [](JSObject* self) -> ESValue* {
            return self->toJSFunction()->protoType();
        },[](::escargot::JSObject* self, ESValue* value){
            if(value->isHeapObject() && value->toHeapObject()->isJSObject())
                self->toJSFunction()->setProtoType(value->toHeapObject()->toJSObject());
        }, true, false, false);
    }
public:
    static JSFunction* create(LexicalEnvironment* outerEnvironment, FunctionNode* functionAST)
    {
        return new JSFunction(outerEnvironment, functionAST);
    }

    ALWAYS_INLINE ESValue* protoType()
    {
        return m_protoType;
    }

    ALWAYS_INLINE void setProtoType(ESValue* obj)
    {
        m_protoType = obj;
    }

    FunctionNode* functionAST() { return m_functionAST; }
    LexicalEnvironment* outerEnvironment() { return m_outerEnvironment; }
protected:
    LexicalEnvironment* m_outerEnvironment;
    FunctionNode* m_functionAST;
    ESValue* m_protoType;
    //JSObject functionObject;
    //HomeObject
    ////JSObject newTarget
    //BindThisValue(V);
    //GetThisBinding();
};


}

#include "ESValueInlines.h"

#endif
