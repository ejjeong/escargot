#ifndef __ScriptParser__
#define __ScriptParser__

namespace escargot {

class ESVMInstance;
class CodeBlock;
class ProgramNode;

struct ParserContextInformation {
    ParserContextInformation(bool strictFromOutside = false, bool shouldWorkAroundIdentifier = true, bool hasArgumentsBinding = false, bool isEvalCode = false, bool isForGlobalScope = false)
        : m_strictFromOutside(strictFromOutside)
        , m_shouldWorkAroundIdentifier(shouldWorkAroundIdentifier)
        , m_hasArgumentsBinding(hasArgumentsBinding)
        , m_isEvalCode(isEvalCode)
        , m_isForGlobalScope(isForGlobalScope)
    {
    }

    bool m_strictFromOutside:1;
    bool m_shouldWorkAroundIdentifier:1;
    bool m_hasArgumentsBinding:1;
    bool m_isEvalCode:1;
    bool m_isForGlobalScope:1;

    size_t hash() const
    {
        // we separate global / non-global code cache
        return m_strictFromOutside | m_shouldWorkAroundIdentifier << 1 | m_hasArgumentsBinding << 2 | m_isEvalCode << 3;
    }
};

class ScriptParser {
public:
    CodeBlock* parseScript(ESVMInstance* instance, escargot::ESString* source, ExecutableType type, ParserContextInformation& parserContextInformation);
    CodeBlock* parseSingleFunction(ESVMInstance* instance, escargot::ESString* argSource, escargot::ESString* bodySource, ParserContextInformation& parserContextInformation);

private:
    struct CodeCacheHash {
    public:
        std::size_t operator()(const std::pair<ESString*, size_t> &x) const
        {
            size_t ret = std::hash<ESString*>()(x.first);
            return ret + x.second;
        }
    };

    struct CodeCacheEqual {
    public:
        bool operator()(const std::pair<ESString*, size_t> &x, const std::pair<ESString*, size_t> &y) const
        {
            return *x.first == *y.first && x.second == y.second;
        }
    };

    void analyzeAST(ESVMInstance* instance, ParserContextInformation& parserContextInformation, ProgramNode* programNode = nullptr);

    std::unordered_map<std::pair<ESString*, size_t>, CodeBlock* , CodeCacheHash, CodeCacheEqual,
    gc_allocator<std::pair<std::pair<ESString*, size_t>, CodeBlock *> > > m_nonGlobalCodeCache;

    std::unordered_map<std::pair<ESString*, size_t>, CodeBlock* , CodeCacheHash, CodeCacheEqual,
    gc_allocator<std::pair<std::pair<ESString*, size_t>, CodeBlock *> > > m_globalCodeCache;
};

}

#endif
