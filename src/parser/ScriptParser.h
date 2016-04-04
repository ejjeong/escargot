#ifndef __ScriptParser__
#define __ScriptParser__

namespace escargot {

class ESVMInstance;
class CodeBlock;
class ProgramNode;

class ScriptParser {
public:
    struct ParserContextInformation {
        ParserContextInformation(bool strictFromOutside = false, bool shouldWorkAroundIdentifier = true)
            : m_strictFromOutside(strictFromOutside)
            , m_shouldWorkAroundIdentifier(shouldWorkAroundIdentifier) { }

        bool m_strictFromOutside:1;
        bool m_shouldWorkAroundIdentifier:1;
    };

    ProgramNode* generateAST(ESVMInstance* instance, escargot::ESString* source, bool isForGlobalScope, ParserContextInformation& parserContextInformation);
    CodeBlock* parseScript(ESVMInstance* instance, escargot::ESString* source, bool isForGlobalScope, CodeBlock::ExecutableType type, ParserContextInformation& parserContextInformation);
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
