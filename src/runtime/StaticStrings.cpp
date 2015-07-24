#include "Escargot.h"

namespace escargot {

namespace strings {

ESString null;
ESString undefined;
ESString prototype;
ESString constructor;
ESString name;
ESString __proto__;

ESString String;
ESString Number;
ESString Object;
ESString Array;
ESString Function;
ESString Empty;

void initStaticStrings()
{
    null = L"null";
    undefined = L"undefined";
    prototype = L"prototype";
    constructor = L"constructor";
    name = L"name";
    __proto__ = L"__proto__";

    String = L"String";
    Number = L"Number";
    Object = L"Object";
    Array = L"Array";
    Function = L"Function";
    Empty = L"Empty";
}

}

}
