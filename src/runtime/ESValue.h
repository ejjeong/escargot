#ifndef ESValue_h
#define ESValue_h
#include "ESString.h"
#include <sys/time.h>
#include <time.h>

namespace escargot {

class Smi;
class HeapObject;
class Undefined;
class Null;
class PBoolean;
class PNumber;
class PString;
class JSObject;
class JSSlot;
class JSFunction;
class JSArray;
class JSString;
class JSDate;
class FunctionNode;
class ESVMInstance;

extern Undefined* esUndefined;
extern Null* esNull;
extern PBoolean* esTrue;
extern PBoolean* esFalse;

extern PNumber* esNaN;
extern PNumber* esInfinity;
extern PNumber* esNegInfinity;
extern PNumber* esMinusZero;

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
    bool isJSSlot() const;
    bool abstractEqualsTo(ESValue* val);
    bool equalsTo(ESValue* val);
    Smi* toSmi() const;
    HeapObject* toHeapObject() const;
    JSSlot* toJSSlot();
    ESString toESString();

    enum PrimitiveTypeHint { PreferString, PreferNumber };
    ESValue* toPrimitive(PrimitiveTypeHint hint = PreferNumber);
    ESValue* toNumber();
    ESValue* toInt32();
    ESValue* toInteger();
    PString* toString();
    ESValue* ensureValue();
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
        Primitive = 1 << 0,
        Undefined = 1 << 1,
        Null = 1 << 2,
        PBoolean = 1 << 3,
        PNumber = 1 << 4,
        PString = 1 << 5,
        JSObject = 1 << 6,
        JSSlot = 1 << 7,
        JSFunction = 1 << 8,
        JSArray = 1 << 9,
        JSString = 1 << 10,
        JSError = 1 << 11,
        JSDate = 1 << 12,
        TypeMask = 0xffff
    };

protected:
    HeapObject(Type type)
    {
        m_data = m_data | type;
    }

public:

    ALWAYS_INLINE Type type() const
    {
        return (Type)(m_data & TypeMask);
    }

    ALWAYS_INLINE bool isPrimitive() const
    {
        return m_data & Type::Primitive;
    }

    ALWAYS_INLINE bool isUndefined() const
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

    ALWAYS_INLINE bool isNull()  const
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

    ALWAYS_INLINE bool isPBoolean() const
    {
        return m_data & Type::PBoolean;
    }

    ALWAYS_INLINE ::escargot::PBoolean* toPBoolean()
    {
#ifndef NDEBUG
        ASSERT(isPBoolean());
#endif
        return reinterpret_cast<::escargot::PBoolean *>(this);
    }

    ALWAYS_INLINE bool isPNumber() const
    {
        return m_data & Type::PNumber;
    }

    ALWAYS_INLINE ::escargot::PNumber* toPNumber()
    {
#ifndef NDEBUG
        ASSERT(isPNumber());
#endif
        return reinterpret_cast<::escargot::PNumber*>(this);
    }

    ALWAYS_INLINE bool isPString() const
    {
        return m_data & Type::PString;
    }

    ALWAYS_INLINE ::escargot::PString* toPString()
    {
#ifndef NDEBUG
        ASSERT(isPString());
#endif
        return reinterpret_cast<::escargot::PString *>(this);
    }

    ALWAYS_INLINE bool isJSObject() const
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
    /*
    ALWAYS_INLINE bool isJSSlot() const
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
    */

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

    ALWAYS_INLINE bool isJSArray() const
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

    ALWAYS_INLINE bool isJSString() const
    {
        return m_data & Type::JSString;
    }

    ALWAYS_INLINE ::escargot::JSString* toJSString()
    {
#ifndef NDEBUG
        ASSERT(isJSString());
#endif
        return reinterpret_cast<::escargot::JSString *>(this);
    }

    ALWAYS_INLINE bool isJSError() const
    {
        return m_data & Type::JSError;
    }

    ALWAYS_INLINE bool isJSDate() const
    {
        return m_data & Type::JSDate;
    }

    ALWAYS_INLINE ::escargot::JSDate* toJSDate()
    {
#ifndef NDEBUG
        ASSERT(isJSDate());
#endif
        return reinterpret_cast<::escargot::JSDate *>(this);
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
        : HeapObject((Type)(Type::Primitive | Type::Undefined))
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
        : HeapObject((Type)(Type::Primitive | Type::Null))
    {

    }
    static Null* create()
    {
        return new Null();
    }
};


class PBoolean : public HeapObject {
protected:
    const int DataMask = 0x80000000;
public:
    PBoolean(bool b)
        : HeapObject((Type)(Type::Primitive | Type::PBoolean))
    {
        set(b);
    }
    static PBoolean* create(bool b)
    {
        return new PBoolean(b);
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

class PNumber : public HeapObject {
public:
    PNumber(double value)
        : HeapObject((Type)(Type::Primitive | Type::PNumber))
    {
        set(value);
    }
    static PNumber* create(double value)
    {
        return new PNumber(value);
    }

    ALWAYS_INLINE void set(double b)
    {
        m_value = b;
    }

    ALWAYS_INLINE double get()
    {
        return m_value;
    }

    ALWAYS_INLINE bool isZero()
    {
        if (m_value == 0 || m_value == -0)
            return true;
        return false;
    }

    ALWAYS_INLINE bool isInfinity()
    {
        if (this == esInfinity || this == esNegInfinity)
            return true;
        return false;
    }

    ALWAYS_INLINE bool isNegative()
    {
        return std::signbit(m_value);
    }

protected:
    double m_value;
};

class PString : public HeapObject {
protected:
    PString(const ESString& src)
        : HeapObject((Type)(Type::Primitive | Type::PString))
    {
        m_string = src;
    }
public:
    static PString* create(const ESString& src)
    {
        return new PString(src);
    }

    const ESString& string()
    {
        return m_string;
    }

    ESValue* length()
    {
        return Smi::fromInt(m_string.length());
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

    friend class DeclarativeEnvironmentRecord;
    //DO NOT USE THIS FUNCITON
    void init(::escargot::ESValue* value,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        HeapObject::m_data = Type::JSSlot;
        m_data.m_value = value;
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = true;
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

    ALWAYS_INLINE void setValue(ESValue* value)
    {
        if(LIKELY(m_isDataProperty)) {
            m_data.m_value = value;
        } else {
            if(m_setter) {
                m_setter(m_data.m_object, value);
            }
        }

    }

    ALWAYS_INLINE ESValue* value()
    {
        if(LIKELY(m_isDataProperty)) {
            return m_data.m_value;
        } else {
            if(m_getter) {
                return m_getter(m_data.m_object);
            }
            return esUndefined;
        }
    }

    ALWAYS_INLINE bool isConfigurable()
    {
        return m_isConfigurable;
    }

    ALWAYS_INLINE bool isEnumerable()
    {
        return m_isEnumerable;
    }

    ALWAYS_INLINE bool isWritable()
    {
        return m_isWritable;
    }

    ALWAYS_INLINE void setConfigurable(bool b)
    {
        m_isConfigurable = b;
    }

    ALWAYS_INLINE void setEnumerable(bool b)
    {
        m_isEnumerable = b;
    }

    ALWAYS_INLINE void setWritable(bool b)
    {
        m_isWritable = b;
    }

    ALWAYS_INLINE void setAsDataProperty()
    {
        m_isDataProperty = true;
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

typedef std::vector<::escargot::JSSlot *, gc_allocator<::escargot::JSSlot *> > JSVectorStd;

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

class JSVector : public JSVectorStd {
public:
    JSVector(size_t siz)
        : JSVectorStd(siz) { }
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
    ESValue* get(const ESAtomicString& key, bool searchPrototype = false)
    {
        if (UNLIKELY(searchPrototype)) {
            JSObject* target = this;
            while(true) {
                ESValue* s = target->get(key);
                if (s != esUndefined)
                    return s;
                ESValue* proto = target->__proto__();
                if (proto && proto->isHeapObject() && proto->toHeapObject()->isJSObject()) {
                    target = proto->toHeapObject()->toJSObject();
                } else {
                   break;
                }
            }
            return esUndefined;
        } else {
            ESValue* ret = esUndefined;
            //TODO Assert: IsPropertyKey(P) is true.
            auto iter = m_map.find(key);
            if(iter != m_map.end()) {
                ret = iter->second->value();
            }
            return ret;
        }
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
        //TODO Assert: Type(Throw) is PBoolean.
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

    ESValue* defaultValue(ESVMInstance* instance, PrimitiveTypeHint hint = PreferNumber);

protected:
    JSObjectMap m_map;
    ESValue* m___proto__;
};

class JSError : public JSObject {
protected:
    JSError(HeapObject::Type type = HeapObject::Type::JSError)
           : JSObject((Type)(Type::JSObject | Type::JSError))
    {

    }

public:
    static JSError* create()
    {
        return new JSError();
    }
};

class JSDate : public JSObject {
protected:
    JSDate(HeapObject::Type type = HeapObject::Type::JSDate)
           : JSObject((Type)(Type::JSObject | Type::JSDate)) {}

public:
    static JSDate* create()
    {
        return new JSDate();
    }

    static JSDate* create(JSObject* proto)
    {
        //TODO
        JSDate* date = new JSDate();
        if(proto != NULL)
            date->set__proto__(proto);
        return date;
    }

    void setTimeValue() {
        gettimeofday(&m_tv, NULL);
    }

    double getTimeAsMilisec() {
        return m_tv.tv_sec*1000 + floor(m_tv.tv_usec/1000);
    }

private:
    struct timeval m_tv;
};

class JSArray : public JSObject {
protected:
    JSArray(HeapObject::Type type = HeapObject::Type::JSArray)
        : JSObject((Type)(Type::JSObject | Type::JSArray))
        , m_vector(16)
        , m_fastmode(true)
    {
        defineAccessorProperty(strings->length, [](JSObject* self) -> ESValue* {
            return self->toJSArray()->length();
        },[](::escargot::JSObject* self, ESValue* value) {
            ESValue* len = value->toInt32();
            self->toJSArray()->setLength(len);
        }, true, false, false);
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
        //if(proto == NULL)
        //    proto = global->arrayPrototype();
        if(proto != NULL)
            arr->set__proto__(proto);
        return arr;
    }

    void set(const ESAtomicString& key, ESValue* val, bool shouldThrowException = false)
    {
        if (m_fastmode) convertToSlowMode();
        m_fastmode = false;
        JSObject::set(key, val, shouldThrowException);
    }

    void set(ESValue* key, ESValue* val, bool shouldThrowException = false)
    {
        int i;
        if (key->isSmi()) {
            i = key->toSmi()->value();
            int len = length()->toSmi()->value();
            if (i == len && m_fastmode) {
                setLength(len+1);
            }
            else if (i >= len) {
                if (m_fastmode) convertToSlowMode();
                setLength(i+1);
            }
        } else {
            if (m_fastmode)
                convertToSlowMode();
        }
        if (m_fastmode) {
            m_vector[i] = escargot::JSSlot::create(val, true, true, true);
        } else {
            JSObject::set(ESAtomicString(key->toESString().data()), val, shouldThrowException);
        }
    }

    ESValue* get(int key)
    {
        if (m_fastmode)
            return m_vector[key]->value();
        return JSObject::get(ESAtomicString(ESString(key).data()));
    }

    ESValue* get(ESValue* key)
    {
        if (m_fastmode && key->isSmi())
            return m_vector[key->toSmi()->value()]->value();
        return JSObject::get(ESAtomicString(key->toESString().data()));
    }

    escargot::JSSlot* find(int key)
    {
        if (m_fastmode)
            return m_vector[key];
        return JSObject::find(ESAtomicString(ESString(key).data()));
    }

    escargot::JSSlot* find(ESValue* key)
    {
        if (m_fastmode && key->isSmi())
            return m_vector[key->toSmi()->value()];
        return JSObject::find(ESAtomicString(key->toESString().data()));
    }

    void push(ESValue* val)
    {
        if (m_fastmode) {
            m_vector.push_back(escargot::JSSlot::create(val, true, true, true));
            int len = length()->toSmi()->value();
            setLength(len + 1);
        } else {
            set(m_length, val);
        }
    }

    void convertToSlowMode()
    {
        m_fastmode = false;
        int len = length()->toSmi()->value();
        if (len == 0) return;
        for (int i = 0; i < len; i++) {
            m_map.insert(std::make_pair(ESAtomicString(ESString(i).data()), m_vector[i]));
        }
        m_vector.clear();
    }

    void setLength(ESValue* len)
    {
        ASSERT(len->isSmi());
        if (len->toSmi()->value() < m_length->toSmi()->value()) {
            //TODO : delete elements
        } else if (m_fastmode && len->toSmi()->value() > m_length->toSmi()->value()) {
            m_vector.resize(len->toSmi()->value());
        }
        m_length = len;
    }

    void setLength(int len)
    {
        ESValue* length = Smi::fromInt(len);
        setLength(length);
    }

    ESValue* length()
    {
        return m_length;
    }

protected:
    ESValue* m_length;
    JSVector m_vector;
    bool m_fastmode;
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

class JSString : public JSObject {
protected:
    JSString(const ESString& str)
        : JSObject((Type)(Type::JSObject | Type::JSString))
    {
        m_stringData = PString::create(str);

        //$21.1.4.1 String.length
        defineAccessorProperty(strings->length, [](JSObject* self) -> ESValue* {
            return self->toJSString()->m_stringData->length();
        }, NULL, true, false, false);
    }

public:
    static JSString* create(const ESString& str)
    {
        return new JSString(str);
    }

    ALWAYS_INLINE ::escargot::PString* getStringData()
    {
        return m_stringData;
    }

private:
    ::escargot::PString* m_stringData;
};

}

#include "ESValueInlines.h"

#endif
