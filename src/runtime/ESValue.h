#ifndef ESValue_h
#define ESValue_h

#include "InternalString.h"

namespace escargot {

class ESUndefined;
class ESNull;
class ESBoolean;
class ESNumberObject;
class ESString;
class ESObject;
class ESSlot;
class ESFunctionObject;
class ESArrayObject;
class ESStringObject;
class ESDateObject;
class FunctionNode;
class ESVMInstance;
class ESPointer;

union ValueDescriptor {
    int64_t asInt64;
#if ESCARGOT_32
    double asDouble;
#elif ESCARGOT_64
    ESPointer* ptr;
#endif
    struct {
        int32_t payload;
        int32_t tag;
    } asBits;
};

#define OBJECT_OFFSETOF(class, field) (reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<class*>(0x4000)->field)) - 0x4000)

#define TagOffset (OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag))
#define PayloadOffset (OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload))

#if ESCARGOT_64
#define CellPayloadOffset 0
#else
#define CellPayloadOffset PayloadOffset
#endif


class ESValue {
    //static void* operator new(size_t, void* p) = delete;
    //static void* operator new[](size_t, void* p) = delete;
    //static void* operator new(size_t size) = delete;
    //static void* operator new[](size_t size) = delete;

public:
#if ESCARGOT_32
    enum { Int32Tag =        0xffffffff };
    enum { BooleanTag =      0xfffffffe };
    enum { NullTag =         0xfffffffd };
    enum { UndefinedTag =    0xfffffffc };
    enum { PointerTag =      0xfffffffb };
    enum { EmptyValueTag =   0xfffffffa };
    enum { DeletedValueTag = 0xfffffff9 };

    enum { LowestTag =  DeletedValueTag };
#endif

    enum ESNullTag { ESNull };
    enum ESUndefinedTag { ESUndefined };
    enum ESTrueTag { ESTrue };
    enum ESFalseTag { ESFalse };
    enum EncodeAsDoubleTag { EncodeAsDouble };

    ESValue();
    ESValue(ESNullTag);
    ESValue(ESUndefinedTag);
    ESValue(ESTrueTag);
    ESValue(ESFalseTag);
    ESValue(ESPointer* ptr);
    ESValue(const ESPointer* ptr);

    // Numbers
    ESValue(EncodeAsDoubleTag, double);
    explicit ESValue(double);
    explicit ESValue(char);
    explicit ESValue(unsigned char);
    explicit ESValue(short);
    explicit ESValue(unsigned short);
    explicit ESValue(int);
    explicit ESValue(unsigned);
    explicit ESValue(long);
    explicit ESValue(unsigned long);
    explicit ESValue(long long);
    explicit ESValue(unsigned long long);

    bool operator==(const ESValue& other) const;
    bool operator!=(const ESValue& other) const;

    bool isInt32() const;
    bool isUInt32() const;
    bool isDouble() const;
    bool isTrue() const;
    bool isFalse() const;

    int32_t asInt32() const;
    uint32_t asUInt32() const;
    int64_t asMachineInt() const;
    double asDouble() const;
    bool asBoolean() const;
    double asNumber() const;

    int32_t asInt32ForArithmetic() const; // Boolean becomes an int, but otherwise like asInt32().

    // Querying the type.
    bool isEmpty() const;
    bool isFunction() const;
    bool isUndefined() const;
    bool isNull() const;
    bool isUndefinedOrNull() const
    {
        return isUndefined() || isNull();
    }
    bool isBoolean() const;
    bool isMachineInt() const;
    bool isNumber() const;
    bool isESString() const;
    bool isSymbol() const;
    bool isPrimitive() const;
    bool isGetterSetter() const;
    bool isCustomGetterSetter() const;
    bool isObject() const;

    // Basic conversions.
    enum PrimitiveTypeHint { PreferString, PreferNumber };
    ESValue toPrimitive(PrimitiveTypeHint = PreferNumber) const;


    bool toBoolean() const;
    double toNumber() const;
    int32_t toInt32() const;
    ESString* asESString() const;
    ESString toESString() const;
    InternalString toInternalString() const;
    ESObject toObject() const;

    bool isESPointer() const;
    ESPointer* asESPointer() const;

    static ptrdiff_t offsetOfPayload() { return OBJECT_OFFSETOF(ESValue, u.asBits.payload); }
    static ptrdiff_t offsetOfTag() { return OBJECT_OFFSETOF(ESValue, u.asBits.tag); }

#if ESCARGOT_32
    uint32_t tag() const;
    int32_t payload() const;
#elif ESCARGOT_64
    // These values are #defines since using static const integers here is a ~1% regression!

    // This value is 2^48, used to encode doubles such that the encoded value will begin
    // with a 16-bit pattern within the range 0x0001..0xFFFE.
    #define DoubleEncodeOffset 0x1000000000000ll
    // If all bits in the mask are set, this indicates an integer number,
    // if any but not all are set this value is a double precision number.
    #define TagTypeNumber 0xffff000000000000ll

    // All non-numeric (bool, null, undefined) immediates have bit 2 set.
    #define TagBitTypeOther 0x2ll
    #define TagBitBool      0x4ll
    #define TagBitUndefined 0x8ll
    // Combined integer value for non-numeric immediates.
    #define ValueFalse     (TagBitTypeOther | TagBitBool | false)
    #define ValueTrue      (TagBitTypeOther | TagBitBool | true)
    #define ValueUndefined (TagBitTypeOther | TagBitUndefined)
    #define ValueNull      (TagBitTypeOther)

    // TagMask is used to check for all types of immediate values (either number or 'other').
    #define TagMask (TagTypeNumber | TagBitTypeOther)

    // These special values are never visible to JavaScript code; Empty is used to represent
    // Array holes, and for uninitialized ESValues. Deleted is used in hash table code.
    // These values would map to cell types in the ESValue encoding, but not valid GC cell
    // pointer should have either of these values (Empty is null, deleted is at an invalid
    // alignment for a GC cell, and in the zero page).
    #define ValueEmpty   0x0ll
    #define ValueDeleted 0x4ll
#endif

private:
    ValueDescriptor u;

public:
    bool isESSlot() const;
    ESSlot* asESSlot();
    bool abstractEqualsTo(const ESValue& val);
    bool equalsTo(const ESValue& val);

    /*
    enum PrimitiveTypeHint { PreferString, PreferNumber };
    ESValue toPrimitive(PrimitiveTypeHint hint = PreferNumber);
    ESValue toInt32();
    ESValue toInteger();
    ESString toString();
    */
    ESValue ensureValue();
};


class ESPointer : public gc {
public:
    enum Type {
        ESString = 1 << 0,
        ESObject = 1 << 1,
        ESSlot = 1 << 2,
        ESFunctionObject = 1 << 3,
        ESArrayObject = 1 << 4,
        ESStringObject = 1 << 5,
        ESErrorObject = 1 << 6,
        ESDateObject = 1 << 7,
        ESNumberObject = 1 << 8,
        TypeMask = 0xffff
    };

protected:
    ESPointer(Type type)
    {
        m_type = type;
    }

public:
    ALWAYS_INLINE int type()
    {
        return m_type;
    }

    ALWAYS_INLINE bool isESString() const
    {
        return m_type & Type::ESString;
    }

    ALWAYS_INLINE ::escargot::ESString* asESString()
    {
#ifndef NDEBUG
        ASSERT(isESString());
#endif
        return reinterpret_cast<::escargot::ESString *>(this);
    }

    ALWAYS_INLINE bool isESObject() const
    {
        return m_type & Type::ESObject;
    }

    ALWAYS_INLINE ::escargot::ESObject* asESObject()
    {
#ifndef NDEBUG
        ASSERT(isESObject());
#endif
        return reinterpret_cast<::escargot::ESObject *>(this);
    }
    ALWAYS_INLINE bool isESSlot() const
    {
        return m_type & Type::ESSlot;
    }

    ALWAYS_INLINE ::escargot::ESSlot* asESSlot()
    {
#ifndef NDEBUG
        ASSERT(isESSlot());
#endif
        return reinterpret_cast<::escargot::ESSlot *>(this);
    }

    ALWAYS_INLINE bool isESFunctionObject()
    {
        return m_type & Type::ESFunctionObject;
    }

    ALWAYS_INLINE ::escargot::ESFunctionObject* asESFunctionObject()
    {
#ifndef NDEBUG
        ASSERT(isESFunctionObject());
#endif
        return reinterpret_cast<::escargot::ESFunctionObject *>(this);
    }

    ALWAYS_INLINE bool isESArrayObject() const
    {
        return m_type & Type::ESArrayObject;
    }

    ALWAYS_INLINE ::escargot::ESArrayObject* asESArrayObject()
    {
#ifndef NDEBUG
        ASSERT(isESArrayObject());
#endif
        return reinterpret_cast<::escargot::ESArrayObject *>(this);
    }

    ALWAYS_INLINE bool isESStringObject() const
    {
        return m_type & Type::ESStringObject;
    }

    ALWAYS_INLINE ::escargot::ESStringObject* asESStringObject()
    {
#ifndef NDEBUG
        ASSERT(isESStringObject());
#endif
        return reinterpret_cast<::escargot::ESStringObject *>(this);
    }

    ALWAYS_INLINE bool isESNumberObject() const
    {
        return m_type & Type::ESNumberObject;
    }

    ALWAYS_INLINE ::escargot::ESNumberObject* asESNumberObject()
    {
#ifndef NDEBUG
        ASSERT(isESNumberObject());
#endif
        return reinterpret_cast<::escargot::ESNumberObject *>(this);
    }

    ALWAYS_INLINE bool isESErrorObject() const
    {
        return m_type & Type::ESErrorObject;
    }

    ALWAYS_INLINE bool isESDateObject() const
    {
        return m_type & Type::ESDateObject;
    }

    ALWAYS_INLINE ::escargot::ESDateObject* asESDateObject()
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
    int m_type;
};


class ESString : public ESPointer {
protected:
    ESString(const InternalString& str)
        : ESPointer(Type::ESString)
    {
        m_string = str;
    }
public:
    static ESString* create(const InternalString& src)
    {
        return new ESString(src);
    }

    const InternalString& string()
    {
        return m_string;
    }

    ESValue length()
    {
        return ESValue(m_string.length());
    }

protected:
    InternalString m_string;
};

class ESSlot : public ESPointer {
    ESSlot(const ::escargot::ESValue& value,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
        : ESPointer(Type::ESSlot)
    {
        m_data = ESValue(value);
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = true;
    }

    ESSlot(::escargot::ESObject* object,
            std::function<ESValue (::escargot::ESObject* obj)> getter = nullptr,
            std::function<void (::escargot::ESObject* obj, const ESValue& value)> setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
        : ESPointer(Type::ESSlot)
    {
        m_data = ESValue((ESPointer *)object);
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = false;
        m_getter = getter;
        m_setter = setter;
    }

    friend class DeclarativeEnvironmentRecord;
    //DO NOT USE THIS FUNCITON
    void init(const ::escargot::ESValue& value,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        m_type = Type::ESSlot;
        m_data = ESValue(value);
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = true;
    }
public:
    static ESSlot* create(const ::escargot::ESValue& value,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        return new ESSlot(value, isWritable, isEnumerable, isConfigurable);
    }

    static ESSlot* create(::escargot::ESObject* object,
            std::function<ESValue (::escargot::ESObject* obj)> getter = nullptr,
            std::function<void (::escargot::ESObject* obj, const ESValue& value)> setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        return new ESSlot(object, getter, setter, isWritable, isEnumerable, isConfigurable);
    }

    ALWAYS_INLINE void setValue(const ::escargot::ESValue& value)
    {
        if(LIKELY(m_isDataProperty)) {
            m_data = value;
        } else {
            if(m_setter) {
                m_setter(m_data.asESPointer()->asESObject(), value);
            }
        }

    }

    ALWAYS_INLINE ESValue value()
    {
        if(LIKELY(m_isDataProperty)) {
            return m_data;
        } else {
            if(m_getter) {
                return m_getter(m_data.asESPointer()->asESObject());
            }
            return ESValue();
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
    ESValue m_data;

    std::function<ESValue (::escargot::ESObject* obj)> m_getter;
    std::function<void (::escargot::ESObject* obj, const ESValue& value)> m_setter;
    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-property-attributes
    bool m_isDataProperty:1;
    bool m_isWritable:1;
    bool m_isEnumerable:1;
    bool m_isConfigurable:1;
};


typedef std::unordered_map<InternalAtomicString, ::escargot::ESSlot* ,
                std::hash<InternalAtomicString>,std::equal_to<InternalAtomicString>,
                gc_allocator<std::pair<const InternalAtomicString, ::escargot::ESSlot* > > > ESObjectMapStd;

typedef std::vector<::escargot::ESSlot* , gc_allocator<::escargot::ESSlot* > > JSVectorStd;

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

class ESObject : public ESPointer {
    friend class ESSlot;
protected:
    ESObject(ESPointer::Type type = ESPointer::Type::ESObject)
        : ESPointer(type)
        , m_map(16)
    {
        //FIXME set proper flags(is...)
        definePropertyOrThrow(strings->constructor, true, false, false);

        defineAccessorProperty(strings->__proto__, [](ESObject* self) -> ESValue {
            return self->__proto__();
        },[](::escargot::ESObject* self, ESValue value){
            if(value.isESPointer() && value.asESPointer()->isESObject()) {
                self->set__proto__(value.asESPointer()->asESObject());
            }
        }, true, false, false);
    }
public:
    void definePropertyOrThrow(const InternalAtomicString& key, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true)
    {
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            m_map.insert(std::make_pair(key, ESSlot::create(ESValue(), isWritable, isEnumerable, isConfigurable)));
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
    ESValue get(const InternalAtomicString& key, bool searchPrototype = false)
    {
        if (UNLIKELY(searchPrototype)) {
            ESObject* target = this;
            while(true) {
                ESValue s = target->get(key);
                if (!s.isUndefined())
                    return s;
                ESValue proto = target->__proto__();
                if (proto.isESPointer() && proto.asESPointer()->isESObject()) {
                    target = proto.asESPointer()->asESObject();
                } else {
                   break;
                }
            }
            return ESValue();
        } else {
            ESValue ret;
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
    void set(const InternalAtomicString& key, const ESValue& val, bool shouldThrowException = false)
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

    void set(ESValue key, const ESValue& val, bool shouldThrowException = false)
    {
        set(InternalAtomicString(key.toInternalString().data()), val, shouldThrowException);
    }

    void defineAccessorProperty(const InternalAtomicString& key,std::function<ESValue (::escargot::ESObject* obj)> getter = nullptr,
            std::function<void (::escargot::ESObject* obj, const ESValue& value)> setter = nullptr,
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

    ALWAYS_INLINE ESValue __proto__()
    {
        return m___proto__;
    }

    ALWAYS_INLINE void set__proto__(const ESValue& obj)
    {
        m___proto__ = obj;
    }

    ALWAYS_INLINE ESValue constructor()
    {
        return get(strings->constructor);
    }

    ALWAYS_INLINE void setConstructor(const ESValue& obj)
    {
        set(strings->constructor, obj);
    }

    ESValue defaultValue(ESVMInstance* instance, ESValue::PrimitiveTypeHint hint = ESValue::PreferNumber);

protected:
    ESObjectMap m_map;
    ESValue m___proto__;
};

class ESErrorObject : public ESObject {
protected:
    ESErrorObject(ESPointer::Type type = ESPointer::Type::ESErrorObject)
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
    ESDateObject(ESPointer::Type type = ESPointer::Type::ESDateObject)
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
    ESArrayObject(ESPointer::Type type = ESPointer::Type::ESArrayObject)
        : ESObject((Type)(Type::ESObject | Type::ESArrayObject))
        , m_vector(16)
        , m_fastmode(true)
    {
        defineAccessorProperty(strings->length, [](ESObject* self) -> ESValue {
            return self->asESArrayObject()->length();
        },[](::escargot::ESObject* self, ESValue value) {
            ESValue len = ESValue(value.asInt32());
            self->asESArrayObject()->setLength(len);
        }, true, false, false);
        m_length = ESValue(0);
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
        if (length == -1)
            arr->convertToSlowMode();
        else
            arr->setLength(length);
        //if(proto == NULL)
        //    proto = global->arrayPrototype();
        if(proto != NULL)
            arr->set__proto__(proto);
        return arr;
    }

    void set(const InternalAtomicString& key, ESValue val, bool shouldThrowException = false)
    {
        if (m_fastmode) convertToSlowMode();
        ESObject::set(key, val, shouldThrowException);
    }

    void set(ESValue key, ESValue val, bool shouldThrowException = false)
    {
        int i;
        if (key.isInt32()) {
            i = key.asInt32();
            int len = length().asInt32();
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
            ESObject::set(InternalAtomicString(key.toInternalString().data()), val, shouldThrowException);
        }
    }

    void set(int i, ESValue val, bool shouldThrowException = false)
    {
        int len = length().asInt32();
        if (i == len && m_fastmode) {
            setLength(len+1);
        }
        else if (i >= len) {
            if (m_fastmode) convertToSlowMode();
            setLength(i+1);
        }
        if (m_fastmode) {
            m_vector[i] = escargot::ESSlot::create(val, true, true, true);
        } else {
            ESObject::set(ESValue(i), val, shouldThrowException);
        }
    }

    ESValue get(int key)
    {
        if (m_fastmode)
            return m_vector[key]->value();
        return ESObject::get(InternalAtomicString(InternalString(key).data()));
    }

    ESValue get(ESValue key)
    {
        if (m_fastmode && key.isInt32())
            return m_vector[key.asInt32()]->value();
        return ESObject::get(InternalAtomicString(key.toInternalString().data()));
    }

    escargot::ESSlot* find(int key)
    {
        if (m_fastmode)
            return m_vector[key];
        return ESObject::find(InternalAtomicString(InternalString(key).data()));
    }

    escargot::ESSlot* find(ESValue key)
    {
        if (m_fastmode && key.isInt32())
            return m_vector[key.asInt32()];
        return ESObject::find(InternalAtomicString(key.toInternalString().data()));
    }

    void push(ESValue val)
    {
        if (m_fastmode) {
            m_vector.push_back(escargot::ESSlot::create(val, true, true, true));
            int len = length().asInt32();
            setLength(len + 1);
        } else {
            set(m_length, val);
        }
    }

    void convertToSlowMode()
    {
        //wprintf(L"CONVERT TO SLOW MODE!!!");
        m_fastmode = false;
        int len = length().asInt32();
        if (len == 0) return;
        for (int i = 0; i < len; i++) {
            m_map.insert(std::make_pair(InternalAtomicString(InternalString(i).data()), m_vector[i]));
        }
        m_vector.clear();
    }

    void setLength(ESValue len)
    {
        ASSERT(len.isInt32());
        if (len.asInt32() < m_length.asInt32()) {
            //TODO : delete elements
        } else if (m_fastmode && len.asInt32() > m_length.asInt32()) {
            m_vector.resize(len.asInt32());
        }
        m_length = len;
    }

    void setLength(int len)
    {
        ESValue length = ESValue(len);
        setLength(length);
    }

    ESValue length()
    {
        return m_length;
    }

protected:
    ESValue m_length;
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
        m_protoType = ESValue();

        defineAccessorProperty(strings->prototype, [](ESObject* self) -> ESValue {
            return self->asESFunctionObject()->protoType();
        },[](::escargot::ESObject* self, ESValue value){
            if(value.isESPointer() && value.asESPointer()->isESObject())
                self->asESFunctionObject()->setProtoType(value.asESPointer()->asESObject());
        }, true, false, false);
    }
public:
    static ESFunctionObject* create(LexicalEnvironment* outerEnvironment, FunctionNode* functionAST)
    {
        return new ESFunctionObject(outerEnvironment, functionAST);
    }

    ALWAYS_INLINE ESValue protoType()
    {
        return m_protoType;
    }

    ALWAYS_INLINE void setProtoType(ESValue obj)
    {
        m_protoType = obj;
    }

    FunctionNode* functionAST() { return m_functionAST; }
    LexicalEnvironment* outerEnvironment() { return m_outerEnvironment; }

    static ESValue call(ESValue callee, ESValue receiver, ESValue arguments[], size_t argumentCount, ESVMInstance* ESVMInstance);
protected:
    LexicalEnvironment* m_outerEnvironment;
    FunctionNode* m_functionAST;
    ESValue m_protoType;
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
        m_stringData = ESString::create(str);

        //$21.1.4.1 String.length
        defineAccessorProperty(strings->length, [](ESObject* self) -> ESValue {
            return self->asESStringObject()->m_stringData->length();
        }, NULL, true, false, false);
    }

public:
    static ESStringObject* create(const InternalString& str)
    {
        return new ESStringObject(str);
    }

    ALWAYS_INLINE ::escargot::ESString* getStringData()
    {
        return m_stringData;
    }

private:
    ::escargot::ESString* m_stringData;
};

class ESNumberObject : public ESObject {
protected:
    ESNumberObject(ESValue value)
        : ESObject((Type)(Type::ESObject | Type::ESNumberObject))
    {
        m_primitiveValue = value;
    }

public:
    static ESNumberObject* create(ESValue value)
    {
        return new ESNumberObject(value);
    }

    ESValue numberData() {
        return m_primitiveValue;
    }

private:
    ESValue m_primitiveValue;
};

}

#include "ESValueInlines.h"

#endif
