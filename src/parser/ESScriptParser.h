#ifndef __ESScriptParser__
#define __ESScriptParser__

#include "ast/AST.h"

namespace escargot {

class ESVMInstance;

class ESScriptParser {
public:

    static Node* parseScript(ESVMInstance* instance, const std::string& cs);
    //TODO
    //static Node* parseScript(const wchar_t* str);
};

}

#endif
