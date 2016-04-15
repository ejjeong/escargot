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

        size_t hash() const
        {
            return m_strictFromOutside | m_shouldWorkAroundIdentifier << 1;
        }
    };

    CodeBlock* parseScript(ESVMInstance* instance, escargot::ESString* source, bool isForGlobalScope, CodeBlock::ExecutableType type, const ParserContextInformation& parserContextInformation);
    CodeBlock* parseSingleFunction(ESVMInstance* instance, escargot::ESString* argSource, escargot::ESString* bodySource, const ParserContextInformation& parserContextInformation);

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

    void analyzeAST(ESVMInstance* instance, bool isForGlobalScope, const ParserContextInformation& parserContextInformation, ProgramNode* programNode = nullptr);

    std::unordered_map<std::pair<ESString*, size_t>, CodeBlock* , CodeCacheHash, CodeCacheEqual,
    gc_allocator<std::pair<std::pair<ESString*, size_t>, CodeBlock *> > > m_nonGlobalCodeCache;

    std::unordered_map<std::pair<ESString*, size_t>, CodeBlock* , CodeCacheHash, CodeCacheEqual,
    gc_allocator<std::pair<std::pair<ESString*, size_t>, CodeBlock *> > > m_globalCodeCache;
};

}

#endif
