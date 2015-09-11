#ifndef StaticStrings_h
#define StaticStrings_h

namespace escargot {

class ESVMInstance;
class ESString;

class Strings {
public:
InternalAtomicString emptyAtomicString;
ESString* emptyESString;

#define ESCARGOT_ASCII_TABLE_MAX 128
ESString* asciiTable[ESCARGOT_ASCII_TABLE_MAX];

ESString* null;
ESString* undefined;
ESString* prototype;
ESString* constructor;
ESString* name;
ESString* arguments;
ESString* length;
InternalAtomicString atomicLength;
ESString* __proto__;

InternalAtomicString atomicName;
InternalAtomicString atomicArguments;

#define ESCARGOT_STRINGS_NUMBERS_MAX 128
InternalAtomicString numbers[ESCARGOT_STRINGS_NUMBERS_MAX];
ESString* nonAtomicNumbers[ESCARGOT_STRINGS_NUMBERS_MAX];

ESString* String;
ESString* Number;
ESString* NaN;
ESString* Infinity;
ESString* NEGATIVE_INFINITY;
ESString* POSITIVE_INFINITY;
ESString* MAX_VALUE;
ESString* MIN_VALUE;
ESString* Object;
ESString* Boolean;
ESString* Error;
ESString* ReferenceError;
ESString* TypeError;
ESString* RangeError;
ESString* SyntaxError;
ESString* message;
ESString* valueOf;
ESString* Array;
ESString* concat;
ESString* indexOf;
ESString* join;
ESString* push;
ESString* pop;
ESString* slice;
ESString* splice;
ESString* shift;
ESString* sort;
ESString* Function;
ESString* Empty;
ESString* Date;
ESString* getDate;
ESString* getDay;
ESString* getFullYear;
ESString* getHours;
ESString* getMinutes;
ESString* getMonth;
ESString* getSeconds;
ESString* getTime;
ESString* getTimezoneOffset;
ESString* setTime;
ESString* Math;
ESString* PI;
ESString* E;
ESString* abs;
ESString* cos;
ESString* ceil;
ESString* max;
ESString* min;
ESString* floor;
ESString* pow;
ESString* random;
ESString* round;
ESString* sin;
ESString* sqrt;
ESString* log;
ESString* toString;
ESString* stringTrue;
ESString* stringFalse;
ESString* boolean;
ESString* number;
ESString* toFixed;
ESString* toPrecision;
ESString* string;
ESString* object;
ESString* function;
ESString* RegExp;
ESString* source;
ESString* test;
ESString* exec;
ESString* input;
ESString* index;

void initStaticStrings(ESVMInstance* instance);
};

extern Strings* strings;

}

#endif
