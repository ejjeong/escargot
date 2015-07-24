#ifndef StaticStrings_h
#define StaticStrings_h

namespace escargot {

namespace strings {

extern ESString null;
extern ESString undefined;
extern ESString prototype;
extern ESString constructor;
extern ESString name;
extern ESString __proto__;

extern ESString String;
extern ESString Number;
extern ESString Object;
extern ESString Array;
extern ESString Function;
extern ESString Empty;

void initStaticStrings();

}

}

#endif
