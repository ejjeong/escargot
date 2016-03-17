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
    struct codecachehash {
    public:
        std::size_t operator()(const std::pair<ESString*, bool> &x) const
        {
            return std::hash<ptrdiff_t>()((ptrdiff_t)x.first + x.second);
        }
    };

    std::unordered_map<std::pair<ESString*, bool>, CodeBlock* , codecachehash, std::equal_to<std::pair<ESString*, bool>>,
    gc_allocator<std::pair<std::pair<ESString*, bool>, CodeBlock *> > > m_nonGlobalCodeCache;

    std::unordered_map<std::pair<ESString*, bool>, CodeBlock* , codecachehash, std::equal_to<std::pair<ESString*, bool>>,
    gc_allocator<std::pair<std::pair<ESString*, bool>, CodeBlock *> > > m_globalCodeCache;
};

}

#endif
