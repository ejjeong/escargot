#ifndef __ScriptParser__
#define __ScriptParser__

namespace escargot {

class ESVMInstance;
class CodeBlock;
class Node;

class ScriptParser {
public:
    Node* generateAST(ESVMInstance* instance, const u16string& cs, bool isForGlobalScope);
    CodeBlock* parseScript(ESVMInstance* instance, const u16string& cs, bool isForGlobalScope);
#ifdef ESCARGOT_PROFILE
    static void dumpStats();
#endif

private:
    std::unordered_map<u16string, CodeBlock* , std::hash<u16string>, std::equal_to<u16string>,
    gc_allocator<std::pair<u16string, CodeBlock *> > > m_nonGlobalCodeCache;

    std::unordered_map<u16string, CodeBlock* , std::hash<u16string>, std::equal_to<u16string>,
    gc_allocator<std::pair<u16string, CodeBlock *> > > m_globalCodeCache;
};

}

#endif



