#ifndef StaticStrings_h
#define StaticStrings_h

namespace escargot {

class ESVMInstance;

class Strings {
public:
InternalString null;
InternalString undefined;
InternalString prototype;
InternalString constructor;
InternalString name;
InternalString arguments;
InternalString length;
InternalAtomicString atomicLength;
InternalString __proto__;

InternalAtomicString atomicName;
InternalAtomicString atomicArguments;

#define ESCARGOT_STRINGS_NUMBERS_MAX 128
InternalAtomicString numbers[ESCARGOT_STRINGS_NUMBERS_MAX];
InternalString nonAtomicNumbers[ESCARGOT_STRINGS_NUMBERS_MAX];

InternalString String;
InternalString Number;
InternalString Object;
InternalString ReferenceError;
InternalString Array;
InternalString concat;
InternalString indexOf;
InternalString join;
InternalString push;
InternalString slice;
InternalString splice;
InternalString sort;
InternalString Function;
InternalString Empty;
InternalString Date;
InternalString getDate;
InternalString getDay;
InternalString getFullYear;
InternalString getHours;
InternalString getMinutes;
InternalString getMonth;
InternalString getSeconds;
InternalString getTime;
InternalString getTimezoneOffset;
InternalString setTime;
InternalString Math;
InternalString PI;
InternalString abs;
InternalString cos;
InternalString ceil;
InternalString max;
InternalString floor;
InternalString pow;
InternalString random;
InternalString round;
InternalString sin;
InternalString sqrt;
InternalString log;
InternalString toString;
InternalString stringTrue;
InternalString stringFalse;
InternalString boolean;
InternalString number;
InternalString string;
InternalString object;
InternalString function;

void initStaticStrings(ESVMInstance* instance);
};

extern Strings* strings;

}

#endif
