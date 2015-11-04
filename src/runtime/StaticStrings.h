#ifndef StaticStrings_h
#define StaticStrings_h

namespace escargot {

class ESVMInstance;
class ESString;

class Strings {
public:
    InternalAtomicString emptyString;

#define ESCARGOT_ASCII_TABLE_MAX 128
    InternalAtomicString asciiTable[ESCARGOT_ASCII_TABLE_MAX];

    InternalAtomicString null;
    InternalAtomicString undefined;
    InternalAtomicString prototype;
    InternalAtomicString constructor;
    InternalAtomicString name;
    InternalAtomicString arguments;
    InternalAtomicString length;
    InternalAtomicString __proto__;

#define ESCARGOT_STRINGS_NUMBERS_MAX 128
    InternalAtomicString numbers[ESCARGOT_STRINGS_NUMBERS_MAX];

    InternalAtomicString String;
    InternalAtomicString Number;
    InternalAtomicString NaN;
    InternalAtomicString Infinity;
    InternalAtomicString NegativeInfinity;
    InternalAtomicString NEGATIVE_INFINITY;
    InternalAtomicString POSITIVE_INFINITY;
    InternalAtomicString MAX_VALUE;
    InternalAtomicString MIN_VALUE;
    InternalAtomicString eval;
    InternalAtomicString Object;
    InternalAtomicString Boolean;
    InternalAtomicString Error;
    InternalAtomicString ReferenceError;
    InternalAtomicString TypeError;
    InternalAtomicString RangeError;
    InternalAtomicString SyntaxError;
    InternalAtomicString message;
    InternalAtomicString valueOf;
    InternalAtomicString Array;
    InternalAtomicString concat;
    InternalAtomicString forEach;
    InternalAtomicString indexOf;
    InternalAtomicString lastIndexOf;
    InternalAtomicString join;
    InternalAtomicString push;
    InternalAtomicString pop;
    InternalAtomicString slice;
    InternalAtomicString splice;
    InternalAtomicString shift;
    InternalAtomicString sort;
    InternalAtomicString Function;
    InternalAtomicString Empty;
    InternalAtomicString Date;
    InternalAtomicString getDate;
    InternalAtomicString getDay;
    InternalAtomicString getFullYear;
    InternalAtomicString getHours;
    InternalAtomicString getMinutes;
    InternalAtomicString getMonth;
    InternalAtomicString getSeconds;
    InternalAtomicString getTime;
    InternalAtomicString getTimezoneOffset;
    InternalAtomicString setTime;
    InternalAtomicString Math;
    InternalAtomicString PI;
    InternalAtomicString E;
    InternalAtomicString abs;
    InternalAtomicString cos;
    InternalAtomicString ceil;
    InternalAtomicString max;
    InternalAtomicString min;
    InternalAtomicString floor;
    InternalAtomicString pow;
    InternalAtomicString random;
    InternalAtomicString round;
    InternalAtomicString sin;
    InternalAtomicString sqrt;
    InternalAtomicString tan;
    InternalAtomicString log;
    InternalAtomicString toString;
    InternalAtomicString toLocaleString;
    InternalAtomicString stringTrue;
    InternalAtomicString stringFalse;
    InternalAtomicString boolean;
    InternalAtomicString number;
    InternalAtomicString toFixed;
    InternalAtomicString toPrecision;
    InternalAtomicString string;
    InternalAtomicString object;
    InternalAtomicString function;
    InternalAtomicString RegExp;
    InternalAtomicString source;
    InternalAtomicString test;
    InternalAtomicString exec;
    InternalAtomicString input;
    InternalAtomicString index;
    InternalAtomicString Int8Array;
    InternalAtomicString Int16Array;
    InternalAtomicString Int32Array;
    InternalAtomicString Uint8Array;
    InternalAtomicString Uint16Array;
    InternalAtomicString Uint32Array;
    InternalAtomicString Uint8ClampedArray;
    InternalAtomicString Float32Array;
    InternalAtomicString Float64Array;
    InternalAtomicString ArrayBuffer;
    InternalAtomicString byteLength;
    InternalAtomicString subarray;
    InternalAtomicString set;
    InternalAtomicString buffer;
    InternalAtomicString JSON;
    InternalAtomicString parse;
    InternalAtomicString stringify;
    InternalAtomicString toJSON;

    void initStaticStrings(ESVMInstance* instance);
};

extern Strings* strings;

}

#endif
