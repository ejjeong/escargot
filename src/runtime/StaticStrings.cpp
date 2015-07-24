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
    null.initHash();
    undefined = L"undefined";
    undefined.initHash();
    prototype = L"prototype";
    prototype.initHash();
    constructor = L"constructor";
    constructor.initHash();
    name = L"name";
    name.initHash();
    __proto__ = L"__proto__";
    __proto__.initHash();

    String = L"String";
    String  .initHash();
    Number = L"Number";
    Number.initHash();
    Object = L"Object";
    Object.initHash();
    Array = L"Array";
    Array.initHash();
    Function = L"Function";
    Function.initHash();
    Empty = L"Empty";
    Empty.initHash();
}

}

}
