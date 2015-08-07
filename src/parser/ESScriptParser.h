#ifndef __ESScriptParser__
#define __ESScriptParser__

#include "ast/AST.h"

class JSContext;
class JSRuntime;

namespace escargot {

class ESVMInstance;

class ESScriptParser {
public:
    static Node* parseScript(ESVMInstance* instance, const std::string& cs);
    //TODO
    //static Node* parseScript(const wchar_t* str);

    static void enter();
    static void exit();

private:
    static std::string parseExternal(std::string& sourceString);
    static ::JSContext* s_cx;
    static ::JSRuntime* s_rt;
    static void* s_global;
};

}

#endif
