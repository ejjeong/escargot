#ifndef StaticStrings_h
#define StaticStrings_h

namespace escargot {

class ESVMInstance;

class Strings {
public:
InternalAtomicString null;
InternalAtomicString undefined;
InternalAtomicString prototype;
InternalAtomicString constructor;
InternalAtomicString name;
InternalAtomicString arguments;
InternalAtomicString length;
InternalAtomicString __proto__;

#define ESCARGOT_STRINGS_NUMBERS_MAX 1024
InternalAtomicString numbers[ESCARGOT_STRINGS_NUMBERS_MAX];
InternalString nonAtomicNumbers[ESCARGOT_STRINGS_NUMBERS_MAX];

InternalAtomicString String;
InternalAtomicString Number;
InternalAtomicString Object;
InternalAtomicString ReferenceError;
InternalAtomicString Array;
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
InternalAtomicString abs;
InternalAtomicString cos;
InternalAtomicString max;
InternalAtomicString floor;
InternalAtomicString pow;
InternalAtomicString random;
InternalAtomicString round;
InternalAtomicString sin;
InternalAtomicString sqrt;
InternalAtomicString toString;

void initStaticStrings(ESVMInstance* instance);
};

extern Strings* strings;

}

#endif
