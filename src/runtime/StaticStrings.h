#ifndef StaticStrings_h
#define StaticStrings_h

namespace escargot {

class ESVMInstance;

class Strings {
public:
ESAtomicString null;
ESAtomicString undefined;
ESAtomicString prototype;
ESAtomicString constructor;
ESAtomicString name;
ESAtomicString arguments;
ESAtomicString length;
ESAtomicString __proto__;
#define ESCARGOT_STRINGS_NUMBERS_MAX 128
ESAtomicString numbers[ESCARGOT_STRINGS_NUMBERS_MAX];

ESAtomicString String;
ESAtomicString Number;
ESAtomicString Object;
ESAtomicString Error;
ESAtomicString Array;
ESAtomicString Function;
ESAtomicString Empty;
void initStaticStrings(ESVMInstance* instance);
};

extern Strings* strings;

}

#endif
