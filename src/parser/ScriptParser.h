#ifndef __ScriptParser__
#define __ScriptParser__

namespace escargot {

class ESVMInstance;
class CodeBlock;
class Node;

class ScriptParser {
public:
    Node* generateAST(ESVMInstance* instance, escargot::ESString* source, bool isForGlobalScope, bool strictFromOutside = false);
    CodeBlock* parseScript(ESVMInstance* instance, escargot::ESString* source, bool isForGlobalScope, bool strictFromOutside = false);
#ifdef ESCARGOT_PROFILE
    static void dumpStats();
#endif

private:
    std::unordered_map<ESString*, CodeBlock* , std::hash<ESString*>, std::equal_to<ESString*>,
    gc_allocator<std::pair<ESString*, CodeBlock *> > > m_nonGlobalCodeCache;

    std::unordered_map<ESString*, CodeBlock* , std::hash<ESString*>, std::equal_to<ESString*>,
    gc_allocator<std::pair<ESString*, CodeBlock *> > > m_globalCodeCache;
};

}

#endif
