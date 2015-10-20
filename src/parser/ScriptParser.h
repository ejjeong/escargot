#ifndef __ScriptParser__
#define __ScriptParser__

namespace escargot {

class ESVMInstance;
class CodeBlock;
class Node;

class ScriptParser {
public:
    static Node* generateAST(ESVMInstance* instance, const u16string& cs);
    static CodeBlock* parseScript(ESVMInstance* instance, const u16string& cs);
#ifdef ESCARGOT_PROFILE
    static void dumpStats();
#endif

private:
};

}

#endif
