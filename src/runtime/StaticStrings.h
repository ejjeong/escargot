#ifndef StaticStrings_h
#define StaticStrings_h

namespace escargot {

class ESVMInstance;
class ESString;

class Strings {
public:
ESString* emptyESString;

#define ESCARGOT_ASCII_TABLE_MAX 128
ESString* asciiTable[ESCARGOT_ASCII_TABLE_MAX];

ESString*  null;
ESString*  undefined;
ESString*  prototype;
ESString*  constructor;
ESString*  name;
ESString*  arguments;
ESString*  length;
InternalAtomicString atomicLength;
ESString*  __proto__;

InternalAtomicString atomicName;
InternalAtomicString atomicArguments;

#define ESCARGOT_STRINGS_NUMBERS_MAX 128
InternalAtomicString numbers[ESCARGOT_STRINGS_NUMBERS_MAX];
ESString*  nonAtomicNumbers[ESCARGOT_STRINGS_NUMBERS_MAX];

ESString*  String;
ESString*  Number;
ESString*  Object;
ESString*  Boolean;
ESString*  Error;
ESString*  ReferenceError;
ESString*  valueOf;
ESString*  Array;
ESString*  concat;
ESString*  indexOf;
ESString*  join;
ESString*  push;
ESString*  slice;
ESString*  splice;
ESString*  sort;
ESString*  Function;
ESString*  Empty;
ESString*  Date;
ESString*  getDate;
ESString*  getDay;
ESString*  getFullYear;
ESString*  getHours;
ESString*  getMinutes;
ESString*  getMonth;
ESString*  getSeconds;
ESString*  getTime;
ESString*  getTimezoneOffset;
ESString*  setTime;
ESString*  Math;
ESString*  PI;
ESString*  abs;
ESString*  cos;
ESString*  ceil;
ESString*  max;
ESString*  floor;
ESString*  pow;
ESString*  random;
ESString*  round;
ESString*  sin;
ESString*  sqrt;
ESString*  log;
ESString*  toString;
ESString*  stringTrue;
ESString*  stringFalse;
ESString*  boolean;
ESString*  number;
ESString*  string;
ESString*  object;
ESString*  function;
ESString*  RegExp;
ESString*  source;
ESString*  test;

void initStaticStrings(ESVMInstance* instance);
};

extern Strings* strings;

}

#endif
