#ifndef ESValue_h
#define ESValue_h
#include "InternalString.h"
#include <sys/time.h>
#include <time.h>

namespace escargot {

class Smi;
class HeapObject;
class ESUndefined;
class ESNull;
class ESBoolean;
class ESNumber;
class PString;
class ESObject;
class ESSlot;
class ESFunctionObject;
class ESArrayObject;
class ESStringObject;
class ESDateObject;
class FunctionNode;
class ESVMInstance;

extern ESUndefined* esUndefined;
extern ESNull* esESNull;
extern ESBoolean* esTrue;
extern ESBoolean* esFalse;

extern ESNumber* esNaN;
extern ESNumber* esInfinity;
extern ESNumber* esNegInfinity;
extern ESNumber* esMinusZero;

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
    bool isESSlot() const;
    bool abstractEqualsTo(ESValue* val);
    bool equalsTo(ESValue* val);
    Smi* toSmi() const;
    HeapObject* toHeapObject() const;
    ESSlot* toESSlot();
    InternalString toInternalString();

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
        ESUndefined = 1 << 1,
        ESNull = 1 << 2,
        ESBoolean = 1 << 3,
        ESNumber = 1 << 4,
        PString = 1 << 5,
        ESObject = 1 << 6,
        ESSlot = 1 << 7,
        ESFunctionObject = 1 << 8,
        ESArrayObject = 1 << 9,
        ESStringObject = 1 << 10,
        ESErrorObject = 1 << 11,
        ESDateObject = 1 << 12,
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

    ALWAYS_INLINE bool isESUndefined() const
    {
        return m_data & Type::ESUndefined;
    }

    ALWAYS_INLINE ::escargot::ESUndefined* toESUndefined()
    {
#ifndef NDEBUG
        ASSERT(isESUndefined());
#endif
        return reinterpret_cast<::escargot::ESUndefined *>(this);
    }

    ALWAYS_INLINE bool isESNull()  const
    {
        return m_data & Type::ESNull;
    }

    ALWAYS_INLINE ::escargot::ESNull* toESNull()
    {
#ifndef NDEBUG
        ASSERT(isESNull());
#endif
        return reinterpret_cast<::escargot::ESNull *>(this);
    }

    ALWAYS_INLINE bool isESBoolean() const
    {
        return m_data & Type::ESBoolean;
    }

    ALWAYS_INLINE ::escargot::ESBoolean* toESBoolean()
    {
#ifndef NDEBUG
        ASSERT(isESBoolean());
#endif
        return reinterpret_cast<::escargot::ESBoolean *>(this);
    }

    ALWAYS_INLINE bool isESNumber() const
    {
        return m_data & Type::ESNumber;
    }

    ALWAYS_INLINE ::escargot::ESNumber* toESNumber()
    {
#ifndef NDEBUG
        ASSERT(isESNumber());
#endif
        return reinterpret_cast<::escargot::ESNumber*>(this);
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

    ALWAYS_INLINE bool isESObject() const
    {
        return m_data & Type::ESObject;
    }

    ALWAYS_INLINE ::escargot::ESObject* toESObject()
    {
#ifndef NDEBUG
        ASSERT(isESObject());
#endif
        return reinterpret_cast<::escargot::ESObject *>(this);
    }
    /*
    ALWAYS_INLINE bool isESSlot() const
    {
        return m_data & Type::ESSlot;
    }

    ALWAYS_INLINE ::escargot::ESSlot* toESSlot()
    {
#ifndef NDEBUG
        ASSERT(isESSlot());
#endif
        return reinterpret_cast<::escargot::ESSlot *>(this);
    }
    */

    ALWAYS_INLINE bool isESFunctionObject()
    {
        return m_data & Type::ESFunctionObject;
    }

    ALWAYS_INLINE ::escargot::ESFunctionObject* toESFunctionObject()
    {
#ifndef NDEBUG
        ASSERT(isESFunctionObject());
#endif
        return reinterpret_cast<::escargot::ESFunctionObject *>(this);
    }

    ALWAYS_INLINE bool isESArrayObject() const
    {
        return m_data & Type::ESArrayObject;
    }

    ALWAYS_INLINE ::escargot::ESArrayObject* toESArrayObject()
    {
#ifndef NDEBUG
        ASSERT(isESArrayObject());
#endif
        return reinterpret_cast<::escargot::ESArrayObject *>(this);
    }

    ALWAYS_INLINE bool isESStringObject() const
    {
        return m_data & Type::ESStringObject;
    }

    ALWAYS_INLINE ::escargot::ESStringObject* toESStringObject()
    {
#ifndef NDEBUG
        ASSERT(isESStringObject());
#endif
        return reinterpret_cast<::escargot::ESStringObject *>(this);
    }

    ALWAYS_INLINE bool isESErrorObject() const
    {
        return m_data & Type::ESErrorObject;
    }

    ALWAYS_INLINE bool isESDateObject() const
    {
        return m_data & Type::ESDateObject;
    }

    ALWAYS_INLINE ::escargot::ESDateObject* toESDateObject()
    {
#ifndef NDEBUG
        ASSERT(isESDateObject());
#endif
        return reinterpret_cast<::escargot::ESDateObject *>(this);
    }

protected:
    // 0x******@@
    // * -> Data
    // @ -> tag
    int m_data;
};

class ESUndefined : public HeapObject {
protected:
public:
    ESUndefined()
        : HeapObject((Type)(Type::Primitive | Type::ESUndefined))
    {

    }
    static ESUndefined* create()
    {
        return new ESUndefined();
    }
};

class ESNull : public HeapObject {
protected:
public:
    ESNull()
        : HeapObject((Type)(Type::Primitive | Type::ESNull))
    {

    }
    static ESNull* create()
    {
        return new ESNull();
    }
};


class ESBoolean : public HeapObject {
protected:
    const int DataMask = 0x80000000;
public:
    ESBoolean(bool b)
        : HeapObject((Type)(Type::Primitive | Type::ESBoolean))
    {
        set(b);
    }
    static ESBoolean* create(bool b)
    {
        return new ESBoolean(b);
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

class ESNumber : public HeapObject {
public:
    ESNumber(double value)
        : HeapObject((Type)(Type::Primitive | Type::ESNumber))
    {
        set(value);
    }
    static ESNumber* create(double value)
    {
        return new ESNumber(value);
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
    PString(const InternalString& src)
        : HeapObject((Type)(Type::Primitive | Type::PString))
    {
        m_string = src;
    }
public:
    static PString* create(const InternalString& src)
    {
        return new PString(src);
    }

    const InternalString& string()
    {
        return m_string;
    }

    ESValue* length()
    {
        return Smi::fromInt(m_string.length());
    }

protected:
    InternalString m_string;
};

class ESSlot : public HeapObject {
    ESSlot(::escargot::ESValue* value,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
        : HeapObject(Type::ESSlot)
    {
        m_data.m_value = value;
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = true;
    }

    ESSlot(::escargot::ESObject* object,
            std::function<ESValue* (::escargot::ESObject* obj)> getter = nullptr,
            std::function<void (::escargot::ESObject* obj, ESValue* value)> setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
        : HeapObject(Type::ESSlot)
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
        HeapObject::m_data = Type::ESSlot;
        m_data.m_value = value;
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = true;
    }
public:
    static ESSlot* create(::escargot::ESValue* value,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        return new ESSlot(value, isWritable, isEnumerable, isConfigurable);
    }

    static ESSlot* create(::escargot::ESObject* object,
            std::function<ESValue* (::escargot::ESObject* obj)> getter = nullptr,
            std::function<void (::escargot::ESObject* obj, ESValue* value)> setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        return new ESSlot(object, getter, setter, isWritable, isEnumerable, isConfigurable);
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
        ::escargot::ESObject* m_object;
    } m_data;

    std::function<ESValue* (::escargot::ESObject* obj)> m_getter;
    std::function<void (::escargot::ESObject* obj, ESValue* value)> m_setter;
    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-property-attributes
    bool m_isDataProperty:1;
    bool m_isWritable:1;
    bool m_isEnumerable:1;
    bool m_isConfigurable:1;
};


typedef std::unordered_map<InternalAtomicString, ::escargot::ESSlot *,
                std::hash<InternalAtomicString>,std::equal_to<InternalAtomicString>,
                gc_allocator<std::pair<const InternalAtomicString, ::escargot::ESSlot *> > > ESObjectMapStd;

typedef std::vector<::escargot::ESSlot *, gc_allocator<::escargot::ESSlot *> > JSVectorStd;

/*
typedef std::map<InternalString, ::escargot::ESSlot *,
            std::less<InternalString>,
            gc_allocator<std::pair<const InternalString, ::escargot::ESSlot *> > > ESObjectMapStd;
*/
class ESObjectMap : public ESObjectMapStd {
public:
    ESObjectMap(size_t siz)
        : ESObjectMapStd(siz) { }

};

class JSVector : public JSVectorStd {
public:
    JSVector(size_t siz)
        : JSVectorStd(siz) { }
};

class ESObject : public HeapObject {
    friend class ESSlot;
protected:
    ESObject(HeapObject::Type type = HeapObject::Type::ESObject)
        : HeapObject(type)
        , m_map(16)
    {
        m___proto__ = esESNull;

        //FIXME set proper flags(is...)
        definePropertyOrThrow(strings->constructor, true, false, false);

        defineAccessorProperty(strings->__proto__, [](ESObject* self) -> ESValue* {
            return self->__proto__();
        },[](::escargot::ESObject* self, ESValue* value){
            if(value->isHeapObject() && value->toHeapObject()->isESObject())
                self->set__proto__(value->toHeapObject()->toESObject());
        }, true, false, false);
    }
public:
    void definePropertyOrThrow(const InternalAtomicString& key, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true)
    {
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            m_map.insert(std::make_pair(key, ESSlot::create(esUndefined, isWritable, isEnumerable, isConfigurable)));
        } else {
            //TODO
        }
    }

    bool hasOwnProperty(const InternalAtomicString& key) {
        auto iter = m_map.find(key);
        if(iter == m_map.end())
            return false;
        return true;
    }

    //$6.1.7.2 Object Internal Methods and Internal Slots
    bool isExtensible() {
        return true;
    }

    static ESObject* create()
    {
        return new ESObject();
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-o-p
    ESValue* get(const InternalAtomicString& key, bool searchPrototype = false)
    {
        if (UNLIKELY(searchPrototype)) {
            ESObject* target = this;
            while(true) {
                ESValue* s = target->get(key);
                if (s != esUndefined)
                    return s;
                ESValue* proto = target->__proto__();
                if (proto && proto->isHeapObject() && proto->toHeapObject()->isESObject()) {
                    target = proto->toHeapObject()->toESObject();
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

    ALWAYS_INLINE escargot::ESSlot* find(const InternalAtomicString& key)
    {
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            return NULL;
        }
        return iter->second;
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
    void set(const InternalAtomicString& key, ESValue* val, bool shouldThrowException = false)
    {
        //TODO Assert: IsPropertyKey(P) is true.
        //TODO Assert: Type(Throw) is ESBoolean.
        //TODO shouldThrowException
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            //TODO set flags
            m_map.insert(std::make_pair(key, escargot::ESSlot::create(val, true, true, true)));
        } else {
            iter->second->setValue(val);
        }
    }

    void set(ESValue* key, ESValue* val, bool shouldThrowException = false)
    {
        set(InternalAtomicString(key->toInternalString().data()), val, shouldThrowException);
    }

    void defineAccessorProperty(const InternalAtomicString& key,std::function<ESValue* (::escargot::ESObject* obj)> getter = nullptr,
            std::function<void (::escargot::ESObject* obj, ESValue* value)> setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        auto iter = m_map.find(key);
        if(iter != m_map.end()) {
            m_map.erase(iter);
        }
        m_map.insert(std::make_pair(key, escargot::ESSlot::create(this, getter, setter, isWritable, isEnumerable, isConfigurable)));

    }

    bool hasKey(const InternalAtomicString& key)
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
    ESObjectMap m_map;
    ESValue* m___proto__;
};

class ESErrorObject : public ESObject {
protected:
    ESErrorObject(HeapObject::Type type = HeapObject::Type::ESErrorObject)
           : ESObject((Type)(Type::ESObject | Type::ESErrorObject))
    {

    }

public:
    static ESErrorObject* create()
    {
        return new ESErrorObject();
    }
};

class ESDateObject : public ESObject {
protected:
    ESDateObject(HeapObject::Type type = HeapObject::Type::ESDateObject)
           : ESObject((Type)(Type::ESObject | Type::ESDateObject)) {}

public:
    static ESDateObject* create()
    {
        return new ESDateObject();
    }

    static ESDateObject* create(ESObject* proto)
    {
        //TODO
        ESDateObject* date = new ESDateObject();
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

class ESArrayObject : public ESObject {
protected:
    ESArrayObject(HeapObject::Type type = HeapObject::Type::ESArrayObject)
        : ESObject((Type)(Type::ESObject | Type::ESArrayObject))
        , m_vector(16)
        , m_fastmode(true)
    {
        defineAccessorProperty(strings->length, [](ESObject* self) -> ESValue* {
            return self->toESArrayObject()->length();
        },[](::escargot::ESObject* self, ESValue* value) {
            ESValue* len = value->toInt32();
            self->toESArrayObject()->setLength(len);
        }, true, false, false);
        m_length = Smi::fromInt(0);
    }
public:
    static ESArrayObject* create()
    {
        return ESArrayObject::create(0);
    }

    // $9.4.2.2
    static ESArrayObject* create(int length, ESObject* proto = NULL)
    {
        //TODO
        ESArrayObject* arr = new ESArrayObject();
        arr->setLength(length);
        //if(proto == NULL)
        //    proto = global->arrayPrototype();
        if(proto != NULL)
            arr->set__proto__(proto);
        return arr;
    }

    void set(const InternalAtomicString& key, ESValue* val, bool shouldThrowException = false)
    {
        if (m_fastmode) convertToSlowMode();
        m_fastmode = false;
        ESObject::set(key, val, shouldThrowException);
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
            m_vector[i] = escargot::ESSlot::create(val, true, true, true);
        } else {
            ESObject::set(InternalAtomicString(key->toInternalString().data()), val, shouldThrowException);
        }
    }

    ESValue* get(int key)
    {
        if (m_fastmode)
            return m_vector[key]->value();
        return ESObject::get(InternalAtomicString(InternalString(key).data()));
    }

    ESValue* get(ESValue* key)
    {
        if (m_fastmode && key->isSmi())
            return m_vector[key->toSmi()->value()]->value();
        return ESObject::get(InternalAtomicString(key->toInternalString().data()));
    }

    escargot::ESSlot* find(int key)
    {
        if (m_fastmode)
            return m_vector[key];
        return ESObject::find(InternalAtomicString(InternalString(key).data()));
    }

    escargot::ESSlot* find(ESValue* key)
    {
        if (m_fastmode && key->isSmi())
            return m_vector[key->toSmi()->value()];
        return ESObject::find(InternalAtomicString(key->toInternalString().data()));
    }

    void push(ESValue* val)
    {
        if (m_fastmode) {
            m_vector.push_back(escargot::ESSlot::create(val, true, true, true));
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
            m_map.insert(std::make_pair(InternalAtomicString(InternalString(i).data()), m_vector[i]));
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
class ESFunctionObject : public ESObject {
protected:
    ESFunctionObject(LexicalEnvironment* outerEnvironment, FunctionNode* functionAST)
        : ESObject((Type)(Type::ESObject | Type::ESFunctionObject))
    {
        m_outerEnvironment = outerEnvironment;
        m_functionAST = functionAST;
        m_protoType = esUndefined;

        defineAccessorProperty(strings->prototype, [](ESObject* self) -> ESValue* {
            return self->toESFunctionObject()->protoType();
        },[](::escargot::ESObject* self, ESValue* value){
            if(value->isHeapObject() && value->toHeapObject()->isESObject())
                self->toESFunctionObject()->setProtoType(value->toHeapObject()->toESObject());
        }, true, false, false);
    }
public:
    static ESFunctionObject* create(LexicalEnvironment* outerEnvironment, FunctionNode* functionAST)
    {
        return new ESFunctionObject(outerEnvironment, functionAST);
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

    static ESValue* call(ESValue* callee, ESValue* receiver, ESValue* arguments[], size_t argumentCount, ESVMInstance* ESVMInstance);
protected:
    LexicalEnvironment* m_outerEnvironment;
    FunctionNode* m_functionAST;
    ESValue* m_protoType;
    //ESObject functionObject;
    //HomeObject
    ////ESObject newTarget
    //BindThisValue(V);
    //GetThisBinding();
};

class ESStringObject : public ESObject {
protected:
    ESStringObject(const InternalString& str)
        : ESObject((Type)(Type::ESObject | Type::ESStringObject))
    {
        m_stringData = PString::create(str);

        //$21.1.4.1 String.length
        defineAccessorProperty(strings->length, [](ESObject* self) -> ESValue* {
            return self->toESStringObject()->m_stringData->length();
        }, NULL, true, false, false);
    }

public:
    static ESStringObject* create(const InternalString& str)
    {
        return new ESStringObject(str);
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
