#ifndef ESValue_h
#define ESValue_h

#include "InternalString.h"

namespace escargot {

class ESUndefined;
class ESNull;
class ESBoolean;
class ESBooleanObject;
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
class ESRegExpObject;

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
    enum ESEmptyValueTag { ESEmptyValue };
    enum ESTrueTag { ESTrue };
    enum ESFalseTag { ESFalse };
    enum EncodeAsDoubleTag { EncodeAsDouble };
    enum ESForceUninitializedTag { ESForceUninitialized };

    ESValue();
    ESValue(ESForceUninitializedTag);
    ESValue(ESNullTag);
    ESValue(ESUndefinedTag);
    ESValue(ESEmptyValueTag);
    ESValue(ESTrueTag);
    ESValue(ESFalseTag);
    ESValue(ESPointer* ptr);
    ESValue(const ESPointer* ptr);

    // Numbers
    ESValue(EncodeAsDoubleTag, double);
    explicit ESValue(double);
    explicit ESValue(bool);
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

    enum PrimitiveTypeHint { PreferString, PreferNumber };
    ESValue toPrimitive(PrimitiveTypeHint = PreferNumber) const; //$7.1.1 ToPrimitive
    bool toBoolean() const; //$7.1.2 ToBoolean
    double toNumber() const; //$7.1.3 ToNumber
    double toInteger() const; //$7.1.4 ToInteger
    int32_t toInt32() const; //$7.1.5 ToInt32
    ESString* toString() const; //$7.1.12 ToString
    ESObject* toObject() const; //$7.1.13 ToObject
    double toLength() const; //$7.1.15 ToLength

    InternalString toInternalString() const;
    ESString* asESString() const;

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
        ESRegExpObject = 1 << 9,
        ESBooleanObject = 1 << 10,
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

    ALWAYS_INLINE bool isESBooleanObject() const
    {
        return m_type & Type::ESBooleanObject;
    }

    ALWAYS_INLINE ::escargot::ESBooleanObject* asESBooleanObject()
    {
#ifndef NDEBUG
        ASSERT(isESBooleanObject());
#endif
        return reinterpret_cast<::escargot::ESBooleanObject *>(this);
    }

    ALWAYS_INLINE bool isESRegExpObject() const
    {
        return m_type & Type::ESRegExpObject;
    }

    ALWAYS_INLINE ::escargot::ESRegExpObject* asESRegExpObject()
    {
#ifndef NDEBUG
        ASSERT(isESRegExpObject());
#endif
        return reinterpret_cast<::escargot::ESRegExpObject *>(this);
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

    int length()
    {
        return m_string.length();
    }

    ESString* substring(int from, int to) const
    {
        ASSERT(0 <= from && from <= to && to <= (int)m_string.length());
        InternalString ret(m_string.string()->substr(from, to-from).c_str());
        return ESString::create(ret);
    }

protected:
    InternalString m_string;
};


struct ESAccessorData : public gc {
public:
    std::function<ESValue (::escargot::ESObject* obj)> m_getter;
    std::function<void (::escargot::ESObject* obj, const ESValue& value)> m_setter;
};

class ESSlot : public ESPointer {
public:
    ESSlot()
        : ESPointer(Type::ESSlot)
    {
        m_isWritable = true;
        m_isEnumerable = true;
        m_isConfigurable = true;
        m_isDataProperty = true;
    }

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
        ESAccessorData* data = new ESAccessorData;
        data->m_getter = getter;
        data->m_setter = setter;
        m_data = ESValue((ESPointer *)data);

        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = false;
    }

    ESSlot(::escargot::ESObject* object,ESAccessorData* data,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
        : ESPointer(Type::ESSlot)
    {
        m_data = ESValue((ESPointer *)data);
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = false;
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

    ALWAYS_INLINE void setValue(const ::escargot::ESValue& value, ::escargot::ESObject* object = NULL)
    {
        if(LIKELY(m_isDataProperty)) {
            m_data = value;
        } else {
            ASSERT(object);
            if(((ESAccessorData *)m_data.asESPointer())->m_setter) {
                ((ESAccessorData *)m_data.asESPointer())->m_setter(object, value);
            }
        }

    }

    ALWAYS_INLINE ESValue value(::escargot::ESObject* object = NULL) const
    {
        if(LIKELY(m_isDataProperty)) {
            return m_data;
        } else {
            ASSERT(object);
            if(((ESAccessorData *)m_data.asESPointer())->m_getter) {
                return ((ESAccessorData *)m_data.asESPointer())->m_getter(object);
            }
            return ESValue();
        }
    }

    ALWAYS_INLINE const ESValue& readDataProperty() const
    {
        ASSERT(m_isDataProperty);
        return m_data;
    }

    ALWAYS_INLINE void setDataProperty(const ::escargot::ESValue& value)
    {
        ASSERT(m_isDataProperty);
        m_data = value;
    }

    ALWAYS_INLINE bool isConfigurable() const
    {
        return m_isConfigurable;
    }

    ALWAYS_INLINE bool isEnumerable() const
    {
        return m_isEnumerable;
    }

    ALWAYS_INLINE bool isWritable() const
    {
        return m_isWritable;
    }

    ALWAYS_INLINE bool isDataProperty() const
    {
        return m_isDataProperty;
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
        m_data = ESValue();
    }

    ALWAYS_INLINE ESAccessorData* accessorData() const
    {
        ASSERT(!m_isDataProperty);
        return (ESAccessorData *)m_data.asESPointer();
    }

protected:
    ESValue m_data;

#pragma pack(push, 1)
    bool m_isDataProperty:1;
    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-property-attributes
    bool m_isWritable:1;
    bool m_isEnumerable:1;
    bool m_isConfigurable:1;
#pragma pack(pop)
};


typedef std::unordered_map<InternalString, ::escargot::ESSlot,
                std::hash<InternalString>,std::equal_to<InternalString>,
                gc_allocator<std::pair<const InternalString, ::escargot::ESSlot> > > ESObjectMapStd;

typedef std::vector<::escargot::ESSlot, gc_allocator<::escargot::ESSlot> > ESVectorStd;

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

class ESVector : public ESVectorStd {
public:
    ESVector(size_t siz)
        : ESVectorStd(siz) { }
};

class ESObject : public ESPointer {
    friend class ESSlot;
protected:
    ESObject(ESPointer::Type type = ESPointer::Type::ESObject);
public:

    //DO NOT USE THIS FUNCTION
    //NOTE rooted ESSlot has short life time.
    escargot::ESSlot* definePropertyOrThrow(const InternalString& key, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true)
    {
        auto iter = m_map.find(key);

        if(iter == m_map.end()) {
            return &m_map.insert(std::make_pair(key, ::escargot::ESSlot(ESValue(), isWritable, isEnumerable, isConfigurable))).first->second;
        } else {
        }

        return &iter->second;
    }

    bool hasOwnProperty(const InternalString& key) {
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
    ESValue get(const InternalString& key, bool searchPrototype = false)
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
            //TODO Assert: IsPropertyKey(P) is true.
            auto iter = m_map.find(key);
            if(iter != m_map.end()) {
                return iter->second.value(this);
            }
            return ESValue();
        }
    }

    //DO NOT USE THIS FUNCTION
    //NOTE rooted ESSlot has short life time.
    ALWAYS_INLINE escargot::ESSlot* find(const InternalString& key)
    {
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            return NULL;
        }
        return &iter->second;
    }

    ALWAYS_INLINE escargot::ESSlot* findUntilPrototype(const InternalString& key)
    {
        ESObject* target = this;
        while(true) {
            auto s = target->find(key);
            if (s)
                return s;
            ESValue proto = target->__proto__();
            if (proto.isESPointer() && proto.asESPointer()->isESObject()) {
                target = proto.asESPointer()->asESObject();
            } else {
               break;
            }
        }
        return NULL;
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
    void set(const InternalString& key, const ESValue& val, bool shouldThrowException = false)
    {
        //TODO Assert: IsPropertyKey(P) is true.
        //TODO Assert: Type(Throw) is ESBoolean.
        //TODO shouldThrowException
        auto iter = m_map.find(key);
        if(iter == m_map.end()) {
            //TODO set flags
            m_map.insert(std::make_pair(key, escargot::ESSlot(val, true, true, true)));
        } else {
            iter->second.setValue(val, this);
        }
    }

    void set(ESValue key, const ESValue& val, bool shouldThrowException = false)
    {
        set(key.toInternalString(), val, shouldThrowException);
    }

    void defineAccessorProperty(const InternalString& key,std::function<ESValue (::escargot::ESObject* obj)> getter = nullptr,
            std::function<void (::escargot::ESObject* obj, const ESValue& value)> setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        auto iter = m_map.find(key);
        if(iter != m_map.end()) {
            m_map.erase(iter);
        }
        m_map.insert(std::make_pair(key, escargot::ESSlot(this, getter, setter, isWritable, isEnumerable, isConfigurable)));
    }

    void defineAccessorProperty(const InternalString& key,ESAccessorData* data,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        auto iter = m_map.find(key);
        if(iter != m_map.end()) {
            m_map.erase(iter);
        }
        m_map.insert(std::make_pair(key, escargot::ESSlot(this, data, isWritable, isEnumerable, isConfigurable)));
    }

    bool hasKey(const InternalString& key)
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
            if(iter->second.isEnumerable()) {
                t((*iter).first,&(*iter).second);
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

    bool hasValueOf()
    {
        if(isESArrayObject())
            return false;
        else
            return true;
    }

    ESValue valueOf();

protected:
    ESObjectMap m_map;
    ESValue m___proto__;
};

class ESErrorObject : public ESObject {
protected:
    ESErrorObject(const InternalString& message = InternalString(&emptyStringData))
           : ESObject((Type)(Type::ESObject | Type::ESErrorObject))
    {
        m_message = message;
    }

public:
    static ESErrorObject* create()
    {
        return new ESErrorObject();
    }

    const InternalString& message() { return m_message; }

protected:
    InternalString m_message;
};

class ReferenceError : public ESErrorObject {
public:
    ReferenceError(const InternalString& message = InternalString(&emptyStringData))
        : ESErrorObject(message)
    {
    }
};

class TypeError : public ESErrorObject {
public:
    TypeError(const InternalString& message = InternalString(&emptyStringData))
        : ESErrorObject(message)
    {
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

    void parseStringToDate(struct tm* timeinfo, const InternalString istr);

    void setTimeValue(ESValue str);

    double getTimeAsMilisec() {
        return m_tv.tv_sec*1000 + floor(m_tv.tv_usec/1000);
    }

    int getDate();
    int getDay();
    int getFullYear();
    int getHours();
    int getMinutes();
    int getMonth();
    int getSeconds();
    int getTimezoneOffset();
    void setTime(double t);

    ESValue valueOf()
    {
        return ESValue(getTimeAsMilisec());
    }

private:
    struct timeval m_tv;
};

class ESArrayObject : public ESObject {
protected:
    ESArrayObject();
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

    void set(const InternalString& key, ESValue val, bool shouldThrowException = false)
    {
        ESObject::set(key, val, shouldThrowException);
    }

    //DO NOT USE RETURN VALUE OF THIS FUNCTION
    escargot::ESSlot* definePropertyOrThrow(ESValue key, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true)
    {
        int i;
        if (key.isInt32()) {
            i = key.asInt32();
            int len = length().asInt32();
            if (i == len && m_fastmode) {
                setLength(len+1);
            }
            else if (i >= len) {
                if (shouldConvertToSlowMode(i)) convertToSlowMode();
                setLength(i+1);
            }
            if (m_fastmode)
                return &m_vector[i];
        }
        return ESObject::definePropertyOrThrow(key.toInternalString().data(), isWritable, isEnumerable, isConfigurable);
    }

    void set(ESValue key, const ESValue& val, bool shouldThrowException = false)
    {
        int i;
        if (key.isInt32()) {
            i = key.asInt32();
            int len = length().asInt32();
            if (i == len && m_fastmode) {
                setLength(len+1);
            }
            else if (i >= len) {
                if (shouldConvertToSlowMode(i)) convertToSlowMode();
                setLength(i+1);
            }
            if (m_fastmode) {
                m_vector[i].setDataProperty(val);
                return;
            }
        }
        ESObject::set(key, val, shouldThrowException);
    }

    void set(int i, const ESValue& val, bool shouldThrowException = false)
    {
        int len = length().asInt32();
        if (i == len && m_fastmode) {
            setLength(len+1);
        }
        else if (i >= len) {
            if (shouldConvertToSlowMode(i)) convertToSlowMode();
            setLength(i+1);
        }
        if (m_fastmode) {
            m_vector[i].setDataProperty(val);
        } else {
            ESObject::set(ESValue(i), val, shouldThrowException);
        }
    }

    ESValue get(int key)
    {
        if (m_fastmode) {
            if(key >= 0 && key < m_length)
                return m_vector[key].readDataProperty();
            else
                return ESValue();
        }
        return ESObject::get(ESValue(key).toInternalString());
    }

    ESValue get(ESValue key)
    {
        if (m_fastmode && key.isInt32()) {
            int idx = key.asInt32();
            if(LIKELY(idx >= 0 && idx < m_length))
                return m_vector[idx].readDataProperty();
            else
                return ESValue();
        }
        return ESObject::get(key.toInternalString());
    }

    //DO NOT USE THIS FUNCTION
    escargot::ESSlot* findOnlyIndex(int key)
    {
        if (LIKELY(m_fastmode && key >= 0 && key < m_length)) {
            return &m_vector[key];
        }
        return NULL;
    }

    //DO NOT USE THIS FUNCTION
    escargot::ESSlot* find(ESValue key)
    {
        if (m_fastmode && key.isInt32()) {
            int idx = key.asInt32();
            if(LIKELY(idx >= 0 && idx < m_length))
                return &m_vector[idx];
            else
                return NULL;
        }
        return ESObject::find(key.toInternalString());
    }

    template <typename Functor>
    void enumeration(Functor t)
    {
        if (m_fastmode) {
            for (int i = 0; i < m_length; i++) {
                //FIXME: check if index i exists or not
                t(ESValue(i), &m_vector[i]);
            }
        }
    }

    void push(const ESValue& val)
    {
        set(m_length, val);
    }

    void insertValue(int idx, const ESValue& val)
    {
        if (m_fastmode) {
            m_vector.insert(m_vector.begin()+idx, val);
            setLength(length().asInt32() + 1);
        } else {
            // TODO
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void eraseValues(int idx, int cnt)
    {
        if (m_fastmode) {
            m_vector.erase(m_vector.begin()+idx, m_vector.begin()+idx+cnt);
        } else {
            for (int k = 0, i = idx; i < length().asInt32() && k < cnt; i++, k++) {
                set(i, get(i+cnt));
            }
        }
        setLength(length().asInt32() - cnt);
    }

    bool shouldConvertToSlowMode(int i) {
        if (m_fastmode && i > MAX_FASTMODE_SIZE)
            return true;
        return false;
    }

    void convertToSlowMode()
    {
        //wprintf(L"CONVERT TO SLOW MODE!!!  \n");
        m_fastmode = false;
        int len = length().asInt32();
        if (len == 0) return;
        for (int i = 0; i < len; i++) {
            m_map.insert(std::make_pair(ESValue(i).toInternalString(), m_vector[i]));
        }
        m_vector.clear();
    }

    void setLength(ESValue len)
    {
        ASSERT(len.isInt32());
        int32_t newLength = len.asInt32();
        if (len.asInt32() < m_length) {
            //TODO : delete elements
        } else if (m_fastmode && newLength > m_length) {
            m_vector.resize(newLength);
        }
        m_length = newLength;
    }

    void setLength(int len)
    {
        ESValue length = ESValue(len);
        setLength(length);
    }

    bool isFastmode()
    {
        return m_fastmode;
    }

    ESValue length()
    {
        return ESValue(m_length);
    }

    void sort()
    {
        RELEASE_ASSERT(isFastmode());
        //TODO non fast mode sort

        std::sort(m_vector.begin(), m_vector.end(),[](const ::escargot::ESSlot& a, const ::escargot::ESSlot& b) -> bool {
            ::escargot::ESString* vala = a.readDataProperty().toString();
            ::escargot::ESString* valb = b.readDataProperty().toString();
            return vala->string() < valb->string();
        });
    }

    template <typename Comp>
    void sort(const Comp& c)
    {
        RELEASE_ASSERT(isFastmode());
        //TODO non fast mode sort

        std::sort(m_vector.begin(), m_vector.end(),c);
    }


protected:
    int32_t m_length;
    ESVector m_vector;
    bool m_fastmode;
    static const int MAX_FASTMODE_SIZE = 65536;
};

class LexicalEnvironment;
class Node;
class ESFunctionObject : public ESObject {
protected:
    ESFunctionObject(LexicalEnvironment* outerEnvironment, FunctionNode* functionAST, ESObject* proto);
public:
    static ESFunctionObject* create(LexicalEnvironment* outerEnvironment, FunctionNode* functionAST, ESObject* proto = NULL)
    {
        ESFunctionObject* ret = new ESFunctionObject(outerEnvironment, functionAST, proto);
        return ret;
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

    static ESValue call(ESValue callee, ESValue receiver, ESValue arguments[], size_t argumentCount, ESVMInstance* ESVMInstance, bool isNewExpression = false);
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
    ESStringObject(const InternalString& str);
public:
    static ESStringObject* create(const InternalString& str)
    {
        return new ESStringObject(str);
    }

    static ESStringObject* create()
    {
        return new ESStringObject("");
    }

    ALWAYS_INLINE ::escargot::ESString* getStringData()
    {
        return m_stringData;
    }

    ALWAYS_INLINE void setString(::escargot::ESString* str)
    {
        m_stringData = str;
    }

    ESValue valueOf()
    {
        return ESString::create(InternalString(m_stringData->string().data()));
    }

private:
    ::escargot::ESString* m_stringData;
};

class ESNumberObject : public ESObject {
protected:
    ESNumberObject(const ESValue& value)
        : ESObject((Type)(Type::ESObject | Type::ESNumberObject))
    {
        m_primitiveValue = value;
    }

public:
    static ESNumberObject* create(const ESValue& value)
    {
        return new ESNumberObject(value);
    }

    ALWAYS_INLINE ESValue numberData() { return m_primitiveValue; }

private:
    ESValue m_primitiveValue;
};

class ESBooleanObject : public ESObject {
protected:
    ESBooleanObject(const ESValue& value)
        : ESObject((Type)(Type::ESObject | Type::ESBooleanObject))
    {
        m_primitiveValue = value;
    }

public:
    static ESBooleanObject* create(const ESValue& value)
    {
        return new ESBooleanObject(value);
    }

    void setBooleanData(const ESValue& value) { m_primitiveValue = value; }
    ALWAYS_INLINE ESValue booleanData() { return m_primitiveValue; }

private:
    ESValue m_primitiveValue;
};

class ESRegExpObject : public ESObject {
public:
    enum Option {
        None = 0,
        Global = 1,
        IgnoreCase = 1 << 1,
        MultiLine = 1 << 2,
        Sticky = 1 << 3,
    };
    static ESRegExpObject* create(const InternalString& source, const Option& option, ESObject* proto = NULL)
    {
        ESRegExpObject* ret = new ESRegExpObject(source, option);
        if (proto != NULL)
            ret->set__proto__(proto);

        return ret;
    }

    //ALWAYS_INLINE escargot::ESString* regExpData() { return m_primitiveValue; }
    //ALWAYS_INLINE void setRegExpData(escargot::ESString* value) { m_primitiveValue = value; }
    ALWAYS_INLINE Option option() { return m_option; }
    ALWAYS_INLINE const InternalString& source() { return m_source; }
    ALWAYS_INLINE const char* utf8Source() { return m_sourceStringAsUtf8; }

    template <typename Func>
    static void prepareForRE2(const char* source,const ESRegExpObject::Option& option, const Func& fn)
    {
        re2::RE2::Options ops;

        if(option & ESRegExpObject::Option::IgnoreCase)
            ops.set_case_sensitive(false);
        if(option & ESRegExpObject::Option::MultiLine)
            ops.set_one_line(false);
        //if(option & ESRegExpObject::Option::Sticky)
        //    ops.set_
        bool isGlobal = option & ESRegExpObject::Option::Global;

        char* sourceForRE2 = (char *)malloc(strlen(source) + 3);
        strcpy(sourceForRE2, "(");
        strcat(sourceForRE2, source);
        strcat(sourceForRE2, ")");
        fn(sourceForRE2, ops, isGlobal);
        free(sourceForRE2);
    }
private:
    ESRegExpObject(const InternalString& source, const Option& option);

    InternalString m_source;
    const char* m_sourceStringAsUtf8;
    Option m_option;
};

}

#include "ESValueInlines.h"

#endif
