/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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
