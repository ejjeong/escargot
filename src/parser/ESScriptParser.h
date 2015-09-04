#ifndef __ESScriptParser__
#define __ESScriptParser__

#include "ast/AST.h"

class JSContext;
class JSRuntime;
class JSObject;
class JSFunction;

namespace escargot {

class ESVMInstance;

class ESScriptParser {
public:
    static ProgramNode* parseScript(ESVMInstance* instance, const escargot::u16string& cs);
    //TODO
    //static Node* parseScript(const wchar_t* str);
#ifdef ESCARGOT_PROFILE
    static void dumpStats();
#endif

private:
    static ::JSContext* s_cx;
    static ::JSRuntime* s_rt;
    static ::JSObject* s_reflectObject;
    static ::JSFunction* s_reflectParseFunction;
    static void* s_global;
};

}

#endif
