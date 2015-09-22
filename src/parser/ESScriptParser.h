#ifndef __ESScriptParser__
#define __ESScriptParser__

#include "ast/AST.h"

namespace escargot {

class ESVMInstance;
class CodeBlock;

class ESScriptParser {
public:
    static CodeBlock* parseScript(ESVMInstance* instance, const escargot::u16string& cs);
#ifdef ESCARGOT_PROFILE
    static void dumpStats();
#endif

private:
};

}

#endif
