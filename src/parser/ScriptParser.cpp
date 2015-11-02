#include "Escargot.h"
#include "parser/ScriptParser.h"
#include "vm/ESVMInstance.h"
#include "runtime/ESValue.h"

#include "ast/AST.h"

#include "esprima.h"

#ifdef ESCARGOT_PROFILE
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#endif

namespace escargot {

#ifdef ESCARGOT_PROFILE
    void ScriptParser::dumpStats()
    {
        unsigned stat;
        auto stream = stderr;

        stat = GC_get_heap_size();
        fwprintf(stream, L"[BOEHM] heap_size: %d\n", stat);
        stat = GC_get_unmapped_bytes();
        fwprintf(stream, L"[BOEHM] unmapped_bytes: %d\n", stat);
        stat = GC_get_total_bytes();
        fwprintf(stream, L"[BOEHM] total_bytes: %d\n", stat);
        stat = GC_get_memory_use();
        fwprintf(stream, L"[BOEHM] memory_use: %d\n", stat);
        stat = GC_get_gc_no();
        fwprintf(stream, L"[BOEHM] gc_no: %d\n", stat);

        struct rusage ru;
        getrusage(RUSAGE_SELF, &ru);
        stat = ru.ru_maxrss;
        fwprintf(stream, L"[LINUX] rss: %d\n", stat);

#if 0
        if (stat > 10000) {
            while (true) {}
        }
#endif
    }
#endif

    Node* ScriptParser::generateAST(ESVMInstance* instance, const escargot::u16string& source, bool isForGlobalScope)
    {
        Node* node;
        try {
            // unsigned long start = ESVMInstance::tickCount();
            node = esprima::parse(source);
            // unsigned long end = ESVMInstance::tickCount();
            // printf("parse takes %lfms\n",(end-start)/1000.0);
        } catch(size_t lineNumber) {
            char temp[512];
            sprintf(temp, "Parse Error %u line", (unsigned)lineNumber);
            throw ESValue(SyntaxError::create(ESString::create(temp)));
        }

        auto markNeedsActivation = [](FunctionNode* nearFunctionNode){
            FunctionNode* node = nearFunctionNode;
            while (node) {
                node->setNeedsActivation(true);
                node = node->outerFunctionNode();
            }
        };

        auto updatePostfixNodeChecker = [](Node* node){
            /*
            if (node && node->type() == NodeType::UpdateExpressionDecrementPostfix) {
            ((UpdateExpressionDecrementPostfixNode *)node)->m_isSimpleCase = true;
            }

            if (node && node->type() == NodeType::UpdateExpressionIncrementPostfix) {
            ((UpdateExpressionIncrementPostfixNode *)node)->m_isSimpleCase = true;
            }
            */
        };

        bool shouldWorkAroundIdentifier = true;
        bool showedEvalInFunction = false;
        std::unordered_map<InternalAtomicString, unsigned, std::hash<InternalAtomicString>, std::equal_to<InternalAtomicString> > knownGlobalNames;

        // fill GlobalData
        if (isForGlobalScope) {
            const ESHiddenClassPropertyInfoVector& info = instance->globalObject()->hiddenClass()->propertyInfo();
            for (unsigned i = 0; i < info.size() ; i ++) {
                if (!info[i].m_flags.m_isDeletedValue) {
                    InternalAtomicString as(instance, info[i].m_name->string());
                    knownGlobalNames.insert(std::make_pair(as, i));
                }
            }
        }

        std::function<void (Node* currentNode,
        std::vector<InternalAtomicStringVector *>& identifierStack,
        FunctionNode* nearFunctionNode)>
        postAnalysisFunction = [&postAnalysisFunction, instance, &markNeedsActivation, &shouldWorkAroundIdentifier, &updatePostfixNodeChecker, &showedEvalInFunction, &knownGlobalNames, &isForGlobalScope]
        (Node* currentNode,
        std::vector<InternalAtomicStringVector *>& identifierStack,
        FunctionNode* nearFunctionNode) {
            if (!currentNode)
            return;
            NodeType type = currentNode->type();
            InternalAtomicStringVector& identifierInCurrentContext = *identifierStack.back();
            if (type == NodeType::Program) {
                StatementNodeVector& v = ((ProgramNode *)currentNode)->m_body;
                for (unsigned i = 0; i < v.size() ; i ++) {
                    postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
                }
            } else if (type == NodeType::VariableDeclaration) {
                VariableDeclaratorVector& v = ((VariableDeclarationNode *)currentNode)->m_declarations;
                for (unsigned i = 0; i < v.size() ; i ++) {
                    postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
                }
            } else if (type == NodeType::VariableDeclarator) {
                // printf("add Identifier %s(var)\n", ((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->nonAtomicName()->utf8Data());
                if (identifierInCurrentContext.end() == std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),
                ((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name())) {
                    identifierInCurrentContext.push_back(((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name());
                }
                if (nearFunctionNode) {
                    // local
                    auto iter = std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),
                    ((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name());
                    ((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->setFastAccessIndex(0, std::distance(identifierInCurrentContext.begin(), iter));
                } else {
                    // global
                    if (isForGlobalScope) {
                        knownGlobalNames.insert(std::make_pair(((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name(), knownGlobalNames.size()));
                    }
                }
            } else if (type == NodeType::FunctionDeclaration) {
                // TODO
                // printf("add Identifier %s(fn)\n", ((FunctionDeclarationNode *)currentNode)->nonAtomicId()->utf8Data());
                ASSERT(identifierInCurrentContext.end() != std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),
                ((FunctionDeclarationNode *)currentNode)->id()));
                // printf("process function body-------------------\n");
                InternalAtomicStringVector newIdentifierVector;
                InternalAtomicStringVector& vec = ((FunctionExpressionNode *)currentNode)->m_params;
                for (unsigned i = 0; i < vec.size() ; i ++) {
                    newIdentifierVector.push_back(vec[i]);
                }
                ((FunctionDeclarationNode *)currentNode)->setOuterFunctionNode(nearFunctionNode);
                identifierStack.push_back(&newIdentifierVector);
                bool preShowedEvalInFunction = showedEvalInFunction;
                postAnalysisFunction(((FunctionDeclarationNode *)currentNode)->m_body, identifierStack, ((FunctionDeclarationNode *)currentNode));
                showedEvalInFunction = preShowedEvalInFunction;
                identifierStack.pop_back();
                ((FunctionDeclarationNode *)currentNode)->setInnerIdentifiers(std::move(newIdentifierVector));
                // printf("end of process function body-------------------\n");
            } else if (type == NodeType::FunctionExpression) {
                // printf("process function body-------------------\n");
                InternalAtomicStringVector newIdentifierVector;
                InternalAtomicStringVector& vec = ((FunctionExpressionNode *)currentNode)->m_params;
                for (unsigned i = 0; i < vec.size() ; i ++) {
                    newIdentifierVector.push_back(vec[i]);
                }
                // If it has own name, should bind function name
                if (((FunctionExpressionNode *)currentNode)->nonAtomicId() != strings->emptyString.string())
                newIdentifierVector.push_back(((FunctionExpressionNode *)currentNode)->id());
                ((FunctionExpressionNode *)currentNode)->setOuterFunctionNode(nearFunctionNode);
                identifierStack.push_back(&newIdentifierVector);
                bool preShowedEvalInFunction = showedEvalInFunction;
                postAnalysisFunction(((FunctionExpressionNode *)currentNode)->m_body, identifierStack, ((FunctionExpressionNode *)currentNode));
                showedEvalInFunction = preShowedEvalInFunction;
                identifierStack.pop_back();
                ((FunctionExpressionNode *)currentNode)->setInnerIdentifiers(std::move(newIdentifierVector));
                // printf("end of process function body-------------------\n");
            } else if (type == NodeType::Identifier) {
                if (!shouldWorkAroundIdentifier || showedEvalInFunction) {
                    return ;
                }
                // use case

                InternalAtomicString name = ((IdentifierNode *)currentNode)->name();
                auto iter = identifierInCurrentContext.end();
                auto riter = identifierInCurrentContext.rbegin(); // std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),name);
                while (riter != identifierInCurrentContext.rend()) {
                    if (*riter == name) {
                        size_t idx = identifierInCurrentContext.size() - 1 - (riter - identifierInCurrentContext.rbegin());
                        iter = identifierInCurrentContext.begin() + idx;
                        break;
                    }
                    riter ++;
                }
                if (identifierInCurrentContext.end() == iter) {
                    // search top...
                    unsigned up = 0;
                    for (int i = identifierStack.size() - 2 ; i >= 0 ; i --) {
                        up++;
                        InternalAtomicStringVector* vector = identifierStack[i];
                        auto iter2 = std::find(vector->begin(),vector->end(),name);
                        if (iter2 != vector->end()) {
                            FunctionNode* fn = nearFunctionNode;
                            for (unsigned j = 0; j < up ; j ++) {
                                fn = fn->outerFunctionNode();
                            }
                            if (fn) {
                                /*printf("outer function of this function  needs capture! -> because fn...%s iden..%s\n",
                                fn->nonAtomicId()->utf8Data(),
                                ((IdentifierNode *)currentNode)->nonAtomicName()->utf8Data());
                                */
                                size_t idx2 = std::distance(vector->begin(), iter2);
                                ((IdentifierNode *)currentNode)->setFastAccessIndex(up, idx2);
                            } else {
                                // fn == global case
                                auto iter = knownGlobalNames.find(name);
                                if (iter != knownGlobalNames.end()) {
                                    ((IdentifierNode *)currentNode)->setGlobalFastAccessIndex(iter->second);
                                }
                            }
                            break;
                        }
                    }
                    if (nearFunctionNode)
                    markNeedsActivation(nearFunctionNode->outerFunctionNode());
                } else {
                    if (nearFunctionNode) {
                        size_t idx = std::distance(identifierInCurrentContext.begin(), iter);
                        ((IdentifierNode *)currentNode)->setFastAccessIndex(0, idx);
                    }
                }
                /*
                if (((IdentifierNode *)currentNode)->canUseFastAccess())
                printf("use Identifier %s %d %d\n"
                , ((IdentifierNode *)currentNode)->nonAtomicName()->utf8Data()
                , (int)((IdentifierNode *)currentNode)->fastAccessUpIndex()
                , (int)((IdentifierNode *)currentNode)->fastAccessIndex()
                );
                else
                printf("use Identifier %s\n", ((IdentifierNode *)currentNode)->nonAtomicName()->utf8Data());
                */
            } else if (type == NodeType::ExpressionStatement) {
                postAnalysisFunction(((ExpressionStatementNode *)currentNode)->m_expression, identifierStack, nearFunctionNode);
                updatePostfixNodeChecker(((ExpressionStatementNode *)currentNode)->m_expression);
            } else if (type >= NodeType::AssignmentExpressionBitwiseAnd && type <= NodeType::AssignmentExpressionUnsignedRightShift) {
                postAnalysisFunction(((AssignmentExpressionBitwiseAndNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
                postAnalysisFunction(((AssignmentExpressionBitwiseAndNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
            } else if (type == NodeType::AssignmentExpressionSimple) {
                postAnalysisFunction(((AssignmentExpressionSimpleNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
                postAnalysisFunction(((AssignmentExpressionSimpleNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
            } else if (type == NodeType::Literal) {
                // DO NOTHING
            }else if (type == NodeType::ArrayExpression) {
                ExpressionNodeVector& v = ((ArrayExpressionNode *)currentNode)->m_elements;
                for (unsigned i = 0; i < v.size() ; i ++) {
                    postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
                }
            } else if (type == NodeType::BlockStatement) {
                StatementNodeVector& v = ((BlockStatementNode *)currentNode)->m_body;
                for (unsigned i = 0; i < v.size() ; i ++) {
                    postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
                }
            } else if (type == NodeType::CallExpression) {

                Node * callee = ((CallExpressionNode *)currentNode)->m_callee;
                if (callee) {
                    if (callee->type() == NodeType::Identifier) {
                        if (((IdentifierNode *)callee)->name() == InternalAtomicString(u"eval")) {
                            markNeedsActivation(nearFunctionNode);
                            showedEvalInFunction = true;
                        }
                    }
                }

                postAnalysisFunction(callee, identifierStack, nearFunctionNode);
                ArgumentVector& v = ((CallExpressionNode *)currentNode)->m_arguments;
                for (unsigned i = 0; i < v.size() ; i ++) {
                    postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
                }
            } else if (type == NodeType::SequenceExpression) {
                ExpressionNodeVector& v = ((SequenceExpressionNode *)currentNode)->m_expressions;
                for (unsigned i = 0; i < v.size() ; i ++) {
                    postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
                }
            } else if (type == NodeType::NewExpression) {
                postAnalysisFunction(((NewExpressionNode *)currentNode)->m_callee, identifierStack, nearFunctionNode);
                ArgumentVector& v = ((NewExpressionNode *)currentNode)->m_arguments;
                for (unsigned i = 0; i < v.size() ; i ++) {
                    postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
                }
            } else if (type == NodeType::ObjectExpression) {
                PropertiesNodeVector& v = ((ObjectExpressionNode *)currentNode)->m_properties;
                for (unsigned i = 0; i < v.size() ; i ++) {
                    PropertyNode* p = v[i];
                    postAnalysisFunction(p->value(), identifierStack, nearFunctionNode);
                    if (p->key()->type() == NodeType::Identifier) {

                    } else {
                        postAnalysisFunction(p->key(), identifierStack, nearFunctionNode);
                    }
                }
            } else if (type == NodeType::ConditionalExpression) {
                postAnalysisFunction(((ConditionalExpressionNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
                postAnalysisFunction(((ConditionalExpressionNode *)currentNode)->m_consequente, identifierStack, nearFunctionNode);
                postAnalysisFunction(((ConditionalExpressionNode *)currentNode)->m_alternate, identifierStack, nearFunctionNode);
            } else if (type == NodeType::Property) {
                postAnalysisFunction(((PropertyNode *)currentNode)->m_key, identifierStack, nearFunctionNode);
                postAnalysisFunction(((PropertyNode *)currentNode)->m_value, identifierStack, nearFunctionNode);
            } else if (type == NodeType::MemberExpression) {
                postAnalysisFunction(((MemberExpressionNode *)currentNode)->m_object, identifierStack, nearFunctionNode);
                postAnalysisFunction(((MemberExpressionNode *)currentNode)->m_property, identifierStack, nearFunctionNode);
            } else if (type >= NodeType::BinaryExpressionBitwiseAnd && type <= NodeType::BinaryExpressionUnsignedRightShift) {
                postAnalysisFunction(((BinaryExpressionBitwiseAndNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
                postAnalysisFunction(((BinaryExpressionBitwiseAndNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
            } else if (type == NodeType::LogicalExpression) {
                postAnalysisFunction(((LogicalExpressionNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
                postAnalysisFunction(((LogicalExpressionNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
            } else if (type >= NodeType::UpdateExpressionDecrementPostfix && type <= UpdateExpressionIncrementPrefix) {
                postAnalysisFunction(((UpdateExpressionDecrementPostfixNode *)currentNode)->m_argument, identifierStack, nearFunctionNode);
            } else if (type >= NodeType::UnaryExpressionBitwiseNot && type <= NodeType::UnaryExpressionVoid) {
                postAnalysisFunction(((UnaryExpressionBitwiseNotNode *)currentNode)->m_argument, identifierStack, nearFunctionNode);
            } else if (type == NodeType::IfStatement) {
                postAnalysisFunction(((IfStatementNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
                postAnalysisFunction(((IfStatementNode *)currentNode)->m_consequente, identifierStack, nearFunctionNode);
                postAnalysisFunction(((IfStatementNode *)currentNode)->m_alternate, identifierStack, nearFunctionNode);
            } else if (type == NodeType::ForStatement) {
                postAnalysisFunction(((ForStatementNode *)currentNode)->m_init, identifierStack, nearFunctionNode);
                postAnalysisFunction(((ForStatementNode *)currentNode)->m_body, identifierStack, nearFunctionNode);
                postAnalysisFunction(((ForStatementNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
                postAnalysisFunction(((ForStatementNode *)currentNode)->m_update, identifierStack, nearFunctionNode);

                updatePostfixNodeChecker(((ForStatementNode *)currentNode)->m_update);
            } else if (type == NodeType::ForInStatement) {
                postAnalysisFunction(((ForInStatementNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
                postAnalysisFunction(((ForInStatementNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
                postAnalysisFunction(((ForInStatementNode *)currentNode)->m_body, identifierStack, nearFunctionNode);
            } else if (type == NodeType::WhileStatement) {
                postAnalysisFunction(((WhileStatementNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
                postAnalysisFunction(((WhileStatementNode *)currentNode)->m_body, identifierStack, nearFunctionNode);
            } else if (type == NodeType::DoWhileStatement) {
                postAnalysisFunction(((DoWhileStatementNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
                postAnalysisFunction(((DoWhileStatementNode *)currentNode)->m_body, identifierStack, nearFunctionNode);
            } else if (type == NodeType::SwitchStatement) {
                postAnalysisFunction(((SwitchStatementNode *)currentNode)->m_discriminant, identifierStack, nearFunctionNode);
                StatementNodeVector& vA =((SwitchStatementNode *)currentNode)->m_casesA;
                for (unsigned i = 0; i < vA.size() ; i ++)
                postAnalysisFunction(vA[i], identifierStack, nearFunctionNode);
                postAnalysisFunction(((SwitchStatementNode *)currentNode)->m_default, identifierStack, nearFunctionNode);
                StatementNodeVector& vB = ((SwitchStatementNode *)currentNode)->m_casesB;
                for (unsigned i = 0; i < vB.size() ; i ++)
                postAnalysisFunction(vB[i], identifierStack, nearFunctionNode);
            } else if (type == NodeType::SwitchCase) {
                postAnalysisFunction(((SwitchCaseNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
                StatementNodeVector& v = ((SwitchCaseNode *)currentNode)->m_consequent;
                for (unsigned i = 0; i < v.size() ; i ++)
                postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
            } else if (type == NodeType::ThisExpression) {

            } else if (type == NodeType::BreakStatement) {
            } else if (type == NodeType::ContinueStatement) {
            } else if (type == NodeType::ReturnStatement) {
                postAnalysisFunction(((ReturnStatmentNode *)currentNode)->m_argument, identifierStack, nearFunctionNode);
            } else if (type == NodeType::EmptyStatement) {
            } else if (type == NodeType::TryStatement) {
                postAnalysisFunction(((TryStatementNode *)currentNode)->m_block, identifierStack, nearFunctionNode);
                bool prevShouldWorkAroundIdentifier = shouldWorkAroundIdentifier;
                shouldWorkAroundIdentifier = false;
                // postAnalysisFunction(((TryStatementNode *)currentNode)->m_handler, identifierStack, nearFunctionNode);
                shouldWorkAroundIdentifier = prevShouldWorkAroundIdentifier;
                postAnalysisFunction(((TryStatementNode *)currentNode)->m_finalizer, identifierStack, nearFunctionNode);
            } else if (type == NodeType::CatchClause) {
                RELEASE_ASSERT_NOT_REACHED();
                /*
                postAnalysisFunction(((CatchClauseNode *)currentNode)->m_param, identifierStack, nearFunctionNode);
                postAnalysisFunction(((CatchClauseNode *)currentNode)->m_guard, identifierStack, nearFunctionNode);
                postAnalysisFunction(((CatchClauseNode *)currentNode)->m_body, identifierStack, nearFunctionNode);
                */
            } else if (type == NodeType::ThrowStatement) {
                postAnalysisFunction(((ThrowStatementNode *)currentNode)->m_argument, identifierStack, nearFunctionNode);
            } else if (type == NodeType::LabeledStatement) {
                postAnalysisFunction(((LabeledStatementNode *)currentNode)->m_statementNode, identifierStack, nearFunctionNode);
            } else if (type == NodeType::BreakLabelStatement) {
            } else if (type == NodeType::ContinueLabelStatement) {
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        };

        InternalAtomicStringVector identifierInCurrentContext;
        std::vector<InternalAtomicStringVector *> stack;
        stack.push_back(&identifierInCurrentContext);
        postAnalysisFunction(node, stack, NULL);
        return node;
    }

    CodeBlock* ScriptParser::parseScript(ESVMInstance* instance, const escargot::u16string& source, bool isForGlobalScope)
    {
        if (source.length() < 1024) {
            if (isForGlobalScope) {
                auto iter = m_globalCodeCache.find(source);
                if (iter != m_globalCodeCache.end()) {
                    return iter->second;
                }
            } else {
                auto iter = m_nonGlobalCodeCache.find(source);
                if (iter != m_nonGlobalCodeCache.end()) {
                    return iter->second;
                }
            }
        }

        Node* node = generateAST(instance, source, isForGlobalScope);
        ASSERT(node->type() == Program);
        CodeBlock* cb = generateByteCode(node);

        if (source.length() < 1024) {
            if (isForGlobalScope) {
                m_globalCodeCache.insert(std::make_pair(source, cb));
            } else {
                m_nonGlobalCodeCache.insert(std::make_pair(source, cb));
            }
        }

        return cb;
    }

}



