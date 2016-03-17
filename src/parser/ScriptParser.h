#ifndef __ScriptParser__
#define __ScriptParser__

namespace escargot {

class ESVMInstance;
class CodeBlock;
class Node;

class ScriptParser {
public:
    Node* generateAST(ESVMInstance* instance, escargot::ESString* source, bool isForGlobalScope, bool strictFromOutside = false);
    CodeBlock* parseScript(ESVMInstance* instance, escargot::ESString* source, bool isForGlobalScope, CodeBlock::ExecutableType type, bool strictFromOutside = false);
#ifdef ESCARGOT_PROFILE
    static void dumpStats();
#endif

private:
    struct CodeCacheHash {
    public:
        std::size_t operator()(const std::pair<ESString*, bool> &x) const
        {
            size_t ret = std::hash<ESString*>()(x.first);
            return ret + x.second;
        }
    };

    struct CodeCacheEqual {
    public:
        bool operator()(const std::pair<ESString*, bool> &x, const std::pair<ESString*, bool> &y) const
        {
            return *x.first == *y.first && x.second == y.second;
        }
    };

    std::unordered_map<std::pair<ESString*, bool>, CodeBlock* , CodeCacheHash, CodeCacheEqual,
    gc_allocator<std::pair<std::pair<ESString*, bool>, CodeBlock *> > > m_nonGlobalCodeCache;

    std::unordered_map<std::pair<ESString*, bool>, CodeBlock* , CodeCacheHash, CodeCacheEqual,
    gc_allocator<std::pair<std::pair<ESString*, bool>, CodeBlock *> > > m_globalCodeCache;
};

}

#endif
