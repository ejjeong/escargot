#ifndef __ESScriptParser__
#define __ESScriptParser__

namespace escargot {

class ESVMInstance;
class CodeBlock;
class Node;

class ESScriptParser {
public:
    static Node* generateAST(ESVMInstance* instance, const escargot::u16string& cs);
    static CodeBlock* parseScript(ESVMInstance* instance, const escargot::u16string& cs);
#ifdef ESCARGOT_PROFILE
    static void dumpStats();
#endif

private:
};

}

#endif
