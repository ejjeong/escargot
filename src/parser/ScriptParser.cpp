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

#include "Escargot.h"
#include "parser/ScriptParser.h"
#include "vm/ESVMInstance.h"
#include "runtime/ESValue.h"

#include "ast/AST.h"

#include "esprima.h"

namespace escargot {

void ScriptParser::analyzeAST(ESVMInstance* instance, ParserContextInformation& parserContextInformation, ProgramNode* programNode)
{
    auto markNeedsActivation = [](FunctionNode* nearFunctionNode)
    {
        FunctionNode* node = nearFunctionNode;
        while (node) {
            node->setNeedsActivation();
            for (size_t i = 0; i < node->innerIdentifiers().size(); i ++) {
                node->innerIdentifiers()[i].m_flags.m_isHeapAllocated = true;
            }
            node = node->outerFunctionNode();
        }
    };

    auto markNeedsHeapAllocatedExecutionContext = [](FunctionNode* nearFunctionNode)
    {
        FunctionNode* node = nearFunctionNode;
        while (node) {
            node->setNeedsHeapAllocatedExecutionContext();
            node = node->outerFunctionNode();
        }
    };

    bool shouldWorkAroundIdentifier = parserContextInformation.m_shouldWorkAroundIdentifier;
    bool isForGlobalScope = parserContextInformation.m_isForGlobalScope;
    std::unordered_map<InternalAtomicString, unsigned, std::hash<InternalAtomicString>, std::equal_to<InternalAtomicString> > knownGlobalNames;
    size_t lastKnownGlobalNameIndex = 0;

    // fill GlobalData
    if (isForGlobalScope) {
        const ESHiddenClassPropertyInfoVector& info = instance->globalObject()->hiddenClass()->propertyInfo();
        for (unsigned i = 0; i < info.size() ; i++) {
            if (!info[i].isDeleted()) {
                InternalAtomicString as;
                if (info[i].name()->isASCIIString()) {
                    as = InternalAtomicString(instance, info[i].name()->asciiData(), info[i].name()->length());
                } else {
                    as = InternalAtomicString(instance, info[i].name()->utf16Data(), info[i].name()->length());
                }
                knownGlobalNames.insert(std::make_pair(as, i));
                // ESCARGOT_LOG_INFO("Initialize knownGlobalNames[%d] = %s", i, as.string()->utf8Data());
            }
        }
        lastKnownGlobalNameIndex = info.size();
    }

    std::function<void(Node* currentNode,
    std::vector<InnerIdentifierInfoVector *>& identifierStack,
    FunctionNode* nearFunctionNode)>
    postAnalysisFunction = [&postAnalysisFunction, &programNode, instance, &markNeedsActivation, &markNeedsHeapAllocatedExecutionContext, &shouldWorkAroundIdentifier, &knownGlobalNames, &lastKnownGlobalNameIndex, &isForGlobalScope]
    (Node* currentNode,
    std::vector<InnerIdentifierInfoVector *>& identifierStack,
    FunctionNode* nearFunctionNode) {
        if (!currentNode)
            return;
        instance->stackCheck();

        NodeType type = currentNode->type();
        InnerIdentifierInfoVector& identifierInCurrentContext = *identifierStack.back();
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
            if (nearFunctionNode) {
                auto name = ((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name();
                if (nearFunctionNode->id() == name) {
                    // Variable shadows function name.
                    size_t functionIdIndex = ((FunctionExpressionNode*)nearFunctionNode)->m_functionIdIndex;
                    if (functionIdIndex != SIZE_MAX) {
                        identifierInCurrentContext[functionIdIndex] = InnerIdentifierInfo(strings->emptyString, InnerIdentifierInfo::Origin::VariableDeclarator);
                        nearFunctionNode->setId(strings->emptyString);
                    }
                }
                // local
                auto iter = identifierInCurrentContext.begin();
                while (iter != identifierInCurrentContext.end()) {
                    if (iter->m_name == name)
                        break;
                    iter++;
                }
                if (identifierInCurrentContext.end() == iter) {
                    identifierInCurrentContext.push_back(InnerIdentifierInfo(((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name(), InnerIdentifierInfo::Origin::VariableDeclarator));
                    iter = identifierInCurrentContext.end() - 1;
                }
                if (shouldWorkAroundIdentifier) {
                    auto dis = std::distance(identifierInCurrentContext.begin(), iter);
                    ((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->setFastAccessIndex(0, dis);
                    iter->m_flags.m_bindingIsImmutable = false;
                }
            } else {
                // global
                if (isForGlobalScope) {
                    auto it = knownGlobalNames.find(((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name());
                    if (it != knownGlobalNames.end()) {
                        ((VariableDeclaratorNode *)currentNode)->setIsGlobalScope(true);
                    } else {
                        // ESCARGOT_LOG_INFO("Update knownGlobalNames[%d] = %s", lastKnownGlobalNameIndex, ((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name().string()->utf8Data());
                        knownGlobalNames.insert(std::make_pair(((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name(), lastKnownGlobalNameIndex++));
                        ((VariableDeclaratorNode *)currentNode)->setIsGlobalScope(true);
                    }
                }
            }
        } else if (type == NodeType::FunctionDeclaration) {
            // TODO
            // printf("add Identifier %s(fn)\n", ((FunctionDeclarationNode *)currentNode)->nonAtomicId()->utf8Data());
            if (nearFunctionNode) {
                auto name = ((FunctionDeclarationNode *)currentNode)->id();
                auto iter = identifierInCurrentContext.begin();
                while (iter != identifierInCurrentContext.end()) {
                    if (iter->m_name == name)
                        break;
                    iter++;
                }
                if (identifierInCurrentContext.end() == iter) {
                    identifierInCurrentContext.push_back(InnerIdentifierInfo(name, InnerIdentifierInfo::Origin::FunctionDeclaration));
                }
            }

            // printf("process function body-------------------\n");
            InnerIdentifierInfoVector* newIdentifierVector = &((FunctionDeclarationNode *)currentNode)->m_innerIdentifiers;
            InternalAtomicStringVector& vec = ((FunctionDeclarationNode *)currentNode)->m_params;
            for (unsigned i = 0; i < vec.size() ; i ++) {
                newIdentifierVector->push_back(InnerIdentifierInfo(vec[i], InnerIdentifierInfo::Origin::Parameter));
                if (vec[i] == strings->arguments)
                    ((FunctionDeclarationNode *)currentNode)->setHasArgumentsBinding();
            }
            ((FunctionDeclarationNode *)currentNode)->setOuterFunctionNode(nearFunctionNode);
            identifierStack.push_back(newIdentifierVector);
            bool preValue = shouldWorkAroundIdentifier;
            postAnalysisFunction(((FunctionDeclarationNode *)currentNode)->m_body, identifierStack, ((FunctionDeclarationNode *)currentNode));
            shouldWorkAroundIdentifier = preValue;
            if (!shouldWorkAroundIdentifier)
                markNeedsActivation((FunctionDeclarationNode *)currentNode);
            identifierStack.pop_back();
            // printf("end of process function body-------------------\n");
        } else if (type == NodeType::FunctionExpression) {
            // printf("process function body-------------------\n");
            InnerIdentifierInfoVector* newIdentifierVector = &((FunctionExpressionNode *)currentNode)->m_innerIdentifiers;
            InternalAtomicStringVector& vec = ((FunctionExpressionNode *)currentNode)->m_params;
            for (unsigned i = 0; i < vec.size(); i ++) {
                newIdentifierVector->push_back(InnerIdentifierInfo(vec[i], InnerIdentifierInfo::Origin::Parameter));
                if (vec[i] == strings->arguments)
                    ((FunctionDeclarationNode *)currentNode)->setHasArgumentsBinding();
            }
            // If it has own name, should bind function name
            if (((FunctionExpressionNode *)currentNode)->id().string()->length() && ((FunctionExpressionNode *)currentNode)->id() != strings->arguments) {

                auto name = ((FunctionExpressionNode *)currentNode)->id();

                bool hasAlready = false;
                for (unsigned i = 0; i < vec.size(); i ++) {
                    if ((*newIdentifierVector)[i].m_name == name) {
                        hasAlready = true;
                        ((FunctionExpressionNode *)currentNode)->m_functionIdIndex = SIZE_MAX;
                        break;
                    }
                }

                if (!hasAlready) {
                    newIdentifierVector->push_back(InnerIdentifierInfo(name, InnerIdentifierInfo::Origin::FunctionExpression));
                    ((FunctionExpressionNode *)currentNode)->m_functionIdIndex = vec.size();
                    (*newIdentifierVector)[vec.size()].m_flags.m_bindingIsImmutable = true;
                }
            }
            ((FunctionExpressionNode *)currentNode)->setOuterFunctionNode(nearFunctionNode);
            identifierStack.push_back(newIdentifierVector);
            bool preValue = shouldWorkAroundIdentifier;
            postAnalysisFunction(((FunctionExpressionNode *)currentNode)->m_body, identifierStack, ((FunctionExpressionNode *)currentNode));
            shouldWorkAroundIdentifier = preValue;
            if (!shouldWorkAroundIdentifier)
                markNeedsActivation((FunctionDeclarationNode *)currentNode);
            identifierStack.pop_back();
            // printf("end of process function body-------------------\n");
        } else if (type == NodeType::Identifier) {
            // use case
            InternalAtomicString name = ((IdentifierNode *)currentNode)->name();

            if (name == strings->arguments) {
                auto iter = identifierInCurrentContext.begin();
                while (iter != identifierInCurrentContext.end()) {
                    if (iter->m_name == strings->arguments) {
                        if (iter->m_flags.m_origin != InnerIdentifierInfo::Origin::VariableDeclarator)
                            break;
                    }
                    iter++;
                }
                if (iter == identifierInCurrentContext.end()) {
                    if (nearFunctionNode) {
                        nearFunctionNode->setNeedsToPrepareGenerateArgumentsObject();
                        for (size_t i = 0; i < nearFunctionNode->innerIdentifiers().size(); i ++) {
                            InnerIdentifierInfo& info = nearFunctionNode->innerIdentifiers()[i];
                            if (info.m_flags.m_origin == InnerIdentifierInfo::Origin::Parameter)
                                info.m_flags.m_isHeapAllocated = true;
                        }
                        markNeedsHeapAllocatedExecutionContext(nearFunctionNode);

                        return;
                    }
                } else {
                    if (nearFunctionNode)
                        nearFunctionNode->setHasArgumentsBinding();
                }
            }

            auto riter = identifierInCurrentContext.rbegin(); // std::find(identifierInCurrentContext.begin(), identifierInCurrentContext.end(), name);
            size_t idx = 0;
            while (riter != identifierInCurrentContext.rend()) {
                if (riter->m_name == name) {
                    break;
                }
                idx++;
                riter++;
            }

            if (identifierInCurrentContext.rend() == riter) {
                // search top...
                unsigned up = 0;
                bool finded = false;
                for (int i = identifierStack.size() - 2 ; i >= 0 ; i --) {
                    up++;
                    InnerIdentifierInfoVector* vector = identifierStack[i];
                    auto iter2 = vector->begin();
                    while (iter2 != vector->end()) {
                        if (iter2->m_name == name)
                            break;
                        iter2++;
                    }
                    if (iter2 != vector->end()) {
                        finded = true;

                        ASSERT(nearFunctionNode);
                        FunctionNode* fn = nearFunctionNode;
                        for (unsigned j = 0; j < up ; j ++) {
                            fn = fn->outerFunctionNode();
                        }

                        size_t idx2 = std::distance(vector->begin(), iter2);
                        ASSERT(fn->innerIdentifiers()[idx2].m_name == name);
                        fn->innerIdentifiers()[idx2].m_flags.m_isHeapAllocated = true;
                        markNeedsHeapAllocatedExecutionContext(nearFunctionNode->outerFunctionNode());
                        if (shouldWorkAroundIdentifier) {
                            ((IdentifierNode *)currentNode)->setFastAccessIndex(up, idx2);
                            if (iter2->m_flags.m_bindingIsImmutable)
                                ((IdentifierNode *)currentNode)->setFastAccessIndexImmutable(true);
                        }
                        /*printf("outer function of this function  needs capture! -> because fn...%s iden..%s\n",
                        fn->nonAtomicId()->utf8Data(),
                        ((IdentifierNode *)currentNode)->nonAtomicName()->utf8Data());
                        */
                        break;
                    }
                }
                if (!finded) {
                    // global case
                    auto iter = knownGlobalNames.find(name);
                    if (iter != knownGlobalNames.end()) {
                        if (shouldWorkAroundIdentifier)
                            ((IdentifierNode *)currentNode)->setGlobalFastAccessIndex(iter->second);
                    } else {
                        if (nearFunctionNode) {
                            // TODO
                            // if we dont have activition flag on this context,
                            // probably we can assume getById only for global object.
                            // so we don't needed HeapAllocatedExecutionContext in this place
                            // markNeedsHeapAllocatedExecutionContext(nearFunctionNode->outerFunctionNode());
                            if (!nearFunctionNode->needsActivation() && isForGlobalScope) {
                                ((IdentifierNode *)currentNode)->m_flags.m_onlySearchGlobal = true;
                            }
                        }
                    }
                }
            } else {
                idx = identifierInCurrentContext.size() - idx - 1;
                if (shouldWorkAroundIdentifier) {
                    ((IdentifierNode *)currentNode)->setFastAccessIndex(0, idx);
                    if (riter->m_flags.m_bindingIsImmutable)
                        ((IdentifierNode *)currentNode)->setFastAccessIndexImmutable(true);
                }
            }
        } else if (type == NodeType::ExpressionStatement) {
            postAnalysisFunction(((ExpressionStatementNode *)currentNode)->m_expression, identifierStack, nearFunctionNode);
        } else if (type >= NodeType::AssignmentExpressionBitwiseAnd && type <= NodeType::AssignmentExpressionUnsignedRightShift) {
            postAnalysisFunction(((AssignmentExpressionBitwiseAndNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
            postAnalysisFunction(((AssignmentExpressionBitwiseAndNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
        } else if (type == NodeType::AssignmentExpressionSimple) {
            postAnalysisFunction(((AssignmentExpressionSimpleNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
            postAnalysisFunction(((AssignmentExpressionSimpleNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
        } else if (type == NodeType::Literal || type == NodeType::RegExpLiteral) {
            // DO NOTHING
        } else if (type == NodeType::ArrayExpression) {
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

            Node* callee = ((CallExpressionNode *)currentNode)->m_callee;
            if (callee) {
                if (callee->type() == NodeType::Identifier) {
                    if (((IdentifierNode *)callee)->name() == strings->eval.string()) {
                        markNeedsActivation(nearFunctionNode);
                        if (nearFunctionNode) {
                            nearFunctionNode->setUsesEval();
                            nearFunctionNode->setNeedsToPrepareGenerateArgumentsObject();
                        }
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
                postAnalysisFunction(p->key(), identifierStack, nearFunctionNode);
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
            FunctionNode* node = nearFunctionNode;
            if (node) {
                node->setNeedsActivation();
                for (size_t i = 0; i < node->innerIdentifiers().size(); i ++) {
                    node->innerIdentifiers()[i].m_flags.m_isHeapAllocated = true;
                }
            }
            markNeedsHeapAllocatedExecutionContext(nearFunctionNode);
            markNeedsActivation(nearFunctionNode);
            postAnalysisFunction(((TryStatementNode *)currentNode)->m_block, identifierStack, nearFunctionNode);
            postAnalysisFunction(((TryStatementNode *)currentNode)->m_handler, identifierStack, nearFunctionNode);
            postAnalysisFunction(((TryStatementNode *)currentNode)->m_finalizer, identifierStack, nearFunctionNode);
        } else if (type == NodeType::CatchClause) {
            bool prevShouldWorkAroundIdentifier = shouldWorkAroundIdentifier;
            shouldWorkAroundIdentifier = false;
            postAnalysisFunction(((CatchClauseNode *)currentNode)->m_param, identifierStack, nearFunctionNode);
            postAnalysisFunction(((CatchClauseNode *)currentNode)->m_guard, identifierStack, nearFunctionNode);
            postAnalysisFunction(((CatchClauseNode *)currentNode)->m_body, identifierStack, nearFunctionNode);
            shouldWorkAroundIdentifier = prevShouldWorkAroundIdentifier;
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

    InnerIdentifierInfoVector identifierInCurrentContext;
    std::vector<InnerIdentifierInfoVector *> stack;
    stack.push_back(&identifierInCurrentContext);
    postAnalysisFunction(programNode, stack, NULL);

    auto calcIDIndex = [](IdentifierNode* idNode, FunctionNode* nearFunctionNode)
    {
        if (idNode->canUseGlobalFastAccess() && nearFunctionNode) {
            FunctionNode* fn = nearFunctionNode;
            while (fn) {
                if (fn->usesEval()) {
                    // Can shadow static declaration
                    idNode->unsetGlobalFastIndex();
                    return;
                }
                fn = fn->outerFunctionNode();
            }
        }
        if (idNode->canUseFastAccess() && nearFunctionNode) {
            if (idNode->fastAccessUpIndex() == 0) {
                if (nearFunctionNode->usesEval()) {
                    // Can shadow static declaration
                    idNode->unsetFastIndex();
                    return;
                }

                size_t heapIndexes = 0;
                size_t stackIndexes = 0;
                auto ids = nearFunctionNode->innerIdentifiers();
                for (unsigned i = 0; i < idNode->fastAccessIndex(); i ++) {
                    if (ids[i].m_flags.m_isHeapAllocated) {
                        heapIndexes++;
                    } else {
                        stackIndexes++;
                    }
                }

                idNode->m_flags.m_isFastAccessIndexIndicatesHeapIndex = ids[idNode->fastAccessIndex()].m_flags.m_isHeapAllocated;
                if (idNode->m_flags.m_isFastAccessIndexIndicatesHeapIndex)
                    idNode->setFastAccessIndex(0, idNode->fastAccessIndex() - stackIndexes);
                else
                    idNode->setFastAccessIndex(0, idNode->fastAccessIndex() - heapIndexes);
            } else {
                FunctionNode* fn = nearFunctionNode;
                for (unsigned j = 0; j < idNode->fastAccessUpIndex() ; j ++) {
                    if (fn->usesEval()) {
                        // Can shadow static declaration
                        idNode->unsetFastIndex();
                        return;
                    }
                    fn = fn->outerFunctionNode();
                }

                if (fn->usesEval()) {
                    idNode->unsetFastIndex();
                    return;
                }

                size_t stackIndexes = 0;
                auto ids = fn->innerIdentifiers();
                for (unsigned i = 0; i < idNode->fastAccessIndex(); i ++) {
                    if (!ids[i].m_flags.m_isHeapAllocated) {
                        stackIndexes++;
                    }
                }

                idNode->m_flags.m_isFastAccessIndexIndicatesHeapIndex = ids[idNode->fastAccessIndex()].m_flags.m_isHeapAllocated;
                idNode->setFastAccessIndex(idNode->fastAccessUpIndex(), idNode->fastAccessIndex() - stackIndexes);
            }
        }
    };

    std::function<void(Node* currentNode, FunctionNode* nearFunctionNode)> postAnalysisFunctionForCalcID = [&postAnalysisFunctionForCalcID, &programNode, instance, &calcIDIndex]
    (Node* currentNode, FunctionNode* nearFunctionNode)
    {
        if (!currentNode)
            return;

        NodeType type = currentNode->type();
        if (type == NodeType::Program) {
            StatementNodeVector& v = ((ProgramNode *)currentNode)->m_body;
            for (unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunctionForCalcID(v[i], nearFunctionNode);
            }
        } else if (type == NodeType::VariableDeclaration) {
            VariableDeclaratorVector& v = ((VariableDeclarationNode *)currentNode)->m_declarations;
            for (unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunctionForCalcID(v[i], nearFunctionNode);
            }
        } else if (type == NodeType::VariableDeclarator) {
            calcIDIndex(((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id), nearFunctionNode);
        } else if (type == NodeType::FunctionDeclaration) {
            if (nearFunctionNode) {
                InternalAtomicString name = ((FunctionDeclarationNode *)currentNode)->m_id;
                auto ids = nearFunctionNode->innerIdentifiers();
                size_t stackIdxCount = 0;
                size_t heapIdxCount = 0;
                for (size_t i = 0; i < ids.size(); i ++) {
                    if (ids[i].m_name == name) {
                        if (ids[i].m_flags.m_isHeapAllocated) {
                            ((FunctionDeclarationNode *)currentNode)->m_functionIdIndexNeedsHeapAllocation = true;
                            ((FunctionDeclarationNode *)currentNode)->m_functionIdIndex = i - stackIdxCount;
                        } else {
                            ((FunctionDeclarationNode *)currentNode)->m_functionIdIndexNeedsHeapAllocation = false;
                            ((FunctionDeclarationNode *)currentNode)->m_functionIdIndex = i - heapIdxCount;
                        }
                        break;
                    } else {
                        if (ids[i].m_flags.m_isHeapAllocated)
                            heapIdxCount++;
                        else
                            stackIdxCount++;
                    }
                }
            }
            postAnalysisFunctionForCalcID(((FunctionDeclarationNode *)currentNode)->m_body, ((FunctionDeclarationNode *)currentNode));
            ((FunctionDeclarationNode *)currentNode)->generateInformationForCodeBlock();
        } else if (type == NodeType::FunctionExpression) {
            postAnalysisFunctionForCalcID(((FunctionExpressionNode *)currentNode)->m_body, ((FunctionExpressionNode *)currentNode));

            if (((FunctionExpressionNode *)currentNode)->m_functionIdIndex != SIZE_MAX) {
                size_t idx = ((FunctionExpressionNode *)currentNode)->m_functionIdIndex;

                ((FunctionExpressionNode *)currentNode)->m_functionIdIndexNeedsHeapAllocation = ((FunctionExpressionNode *)currentNode)->m_innerIdentifiers[idx].m_flags.m_isHeapAllocated;

                size_t heapCnt = 0;
                size_t stackCnt = 0;
                for (size_t i = 0; i < idx; i++) {
                    if (((FunctionExpressionNode *)currentNode)->m_innerIdentifiers[i].m_flags.m_isHeapAllocated)
                        heapCnt++;
                    else
                        stackCnt++;
                }

                if (((FunctionExpressionNode *)currentNode)->m_functionIdIndexNeedsHeapAllocation) {
                    ((FunctionExpressionNode *)currentNode)->m_functionIdIndex = idx - stackCnt;
                } else {
                    ((FunctionExpressionNode *)currentNode)->m_functionIdIndex = idx - heapCnt;
                }
            }

            ((FunctionExpressionNode *)currentNode)->generateInformationForCodeBlock();
        } else if (type == NodeType::Identifier) {
            calcIDIndex((IdentifierNode *)currentNode, nearFunctionNode);
        } else if (type == NodeType::ExpressionStatement) {
            postAnalysisFunctionForCalcID(((ExpressionStatementNode *)currentNode)->m_expression, nearFunctionNode);
        } else if (type >= NodeType::AssignmentExpressionBitwiseAnd && type <= NodeType::AssignmentExpressionUnsignedRightShift) {
            postAnalysisFunctionForCalcID(((AssignmentExpressionBitwiseAndNode *)currentNode)->m_right, nearFunctionNode);
            postAnalysisFunctionForCalcID(((AssignmentExpressionBitwiseAndNode *)currentNode)->m_left, nearFunctionNode);
        } else if (type == NodeType::AssignmentExpressionSimple) {
            postAnalysisFunctionForCalcID(((AssignmentExpressionSimpleNode *)currentNode)->m_right, nearFunctionNode);
            postAnalysisFunctionForCalcID(((AssignmentExpressionSimpleNode *)currentNode)->m_left, nearFunctionNode);
        } else if (type == NodeType::Literal || type == NodeType::RegExpLiteral) {
            // DO NOTHING
        } else if (type == NodeType::ArrayExpression) {
            ExpressionNodeVector& v = ((ArrayExpressionNode *)currentNode)->m_elements;
            for (unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunctionForCalcID(v[i], nearFunctionNode);
            }
        } else if (type == NodeType::BlockStatement) {
            StatementNodeVector& v = ((BlockStatementNode *)currentNode)->m_body;
            for (unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunctionForCalcID(v[i], nearFunctionNode);
            }
        } else if (type == NodeType::CallExpression) {
            Node* callee = ((CallExpressionNode *)currentNode)->m_callee;
            postAnalysisFunctionForCalcID(callee, nearFunctionNode);
            ArgumentVector& v = ((CallExpressionNode *)currentNode)->m_arguments;
            for (unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunctionForCalcID(v[i], nearFunctionNode);
            }
        } else if (type == NodeType::SequenceExpression) {
            ExpressionNodeVector& v = ((SequenceExpressionNode *)currentNode)->m_expressions;
            for (unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunctionForCalcID(v[i], nearFunctionNode);
            }
        } else if (type == NodeType::NewExpression) {
            postAnalysisFunctionForCalcID(((NewExpressionNode *)currentNode)->m_callee, nearFunctionNode);
            ArgumentVector& v = ((NewExpressionNode *)currentNode)->m_arguments;
            for (unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunctionForCalcID(v[i], nearFunctionNode);
            }
        } else if (type == NodeType::ObjectExpression) {
            PropertiesNodeVector& v = ((ObjectExpressionNode *)currentNode)->m_properties;
            for (unsigned i = 0; i < v.size() ; i ++) {
                PropertyNode* p = v[i];
                postAnalysisFunctionForCalcID(p->value(), nearFunctionNode);
                if (p->key()->type() == NodeType::Identifier) {

                } else {
                    postAnalysisFunctionForCalcID(p->key(), nearFunctionNode);
                }
            }
        } else if (type == NodeType::ConditionalExpression) {
            postAnalysisFunctionForCalcID(((ConditionalExpressionNode *)currentNode)->m_test, nearFunctionNode);
            postAnalysisFunctionForCalcID(((ConditionalExpressionNode *)currentNode)->m_consequente, nearFunctionNode);
            postAnalysisFunctionForCalcID(((ConditionalExpressionNode *)currentNode)->m_alternate, nearFunctionNode);
        } else if (type == NodeType::Property) {
            postAnalysisFunctionForCalcID(((PropertyNode *)currentNode)->m_key, nearFunctionNode);
            postAnalysisFunctionForCalcID(((PropertyNode *)currentNode)->m_value, nearFunctionNode);
        } else if (type == NodeType::MemberExpression) {
            postAnalysisFunctionForCalcID(((MemberExpressionNode *)currentNode)->m_object, nearFunctionNode);
            postAnalysisFunctionForCalcID(((MemberExpressionNode *)currentNode)->m_property, nearFunctionNode);
        } else if (type >= NodeType::BinaryExpressionBitwiseAnd && type <= NodeType::BinaryExpressionUnsignedRightShift) {
            postAnalysisFunctionForCalcID(((BinaryExpressionBitwiseAndNode *)currentNode)->m_right, nearFunctionNode);
            postAnalysisFunctionForCalcID(((BinaryExpressionBitwiseAndNode *)currentNode)->m_left, nearFunctionNode);
        } else if (type >= NodeType::UpdateExpressionDecrementPostfix && type <= UpdateExpressionIncrementPrefix) {
            postAnalysisFunctionForCalcID(((UpdateExpressionDecrementPostfixNode *)currentNode)->m_argument, nearFunctionNode);
        } else if (type >= NodeType::UnaryExpressionBitwiseNot && type <= NodeType::UnaryExpressionVoid) {
            postAnalysisFunctionForCalcID(((UnaryExpressionBitwiseNotNode *)currentNode)->m_argument, nearFunctionNode);
        } else if (type == NodeType::IfStatement) {
            postAnalysisFunctionForCalcID(((IfStatementNode *)currentNode)->m_test, nearFunctionNode);
            postAnalysisFunctionForCalcID(((IfStatementNode *)currentNode)->m_consequente, nearFunctionNode);
            postAnalysisFunctionForCalcID(((IfStatementNode *)currentNode)->m_alternate, nearFunctionNode);
        } else if (type == NodeType::ForStatement) {
            postAnalysisFunctionForCalcID(((ForStatementNode *)currentNode)->m_init, nearFunctionNode);
            postAnalysisFunctionForCalcID(((ForStatementNode *)currentNode)->m_body, nearFunctionNode);
            postAnalysisFunctionForCalcID(((ForStatementNode *)currentNode)->m_test, nearFunctionNode);
            postAnalysisFunctionForCalcID(((ForStatementNode *)currentNode)->m_update, nearFunctionNode);
        } else if (type == NodeType::ForInStatement) {
            postAnalysisFunctionForCalcID(((ForInStatementNode *)currentNode)->m_left, nearFunctionNode);
            postAnalysisFunctionForCalcID(((ForInStatementNode *)currentNode)->m_right, nearFunctionNode);
            postAnalysisFunctionForCalcID(((ForInStatementNode *)currentNode)->m_body, nearFunctionNode);
        } else if (type == NodeType::WhileStatement) {
            postAnalysisFunctionForCalcID(((WhileStatementNode *)currentNode)->m_test, nearFunctionNode);
            postAnalysisFunctionForCalcID(((WhileStatementNode *)currentNode)->m_body, nearFunctionNode);
        } else if (type == NodeType::DoWhileStatement) {
            postAnalysisFunctionForCalcID(((DoWhileStatementNode *)currentNode)->m_test, nearFunctionNode);
            postAnalysisFunctionForCalcID(((DoWhileStatementNode *)currentNode)->m_body, nearFunctionNode);
        } else if (type == NodeType::SwitchStatement) {
            postAnalysisFunctionForCalcID(((SwitchStatementNode *)currentNode)->m_discriminant, nearFunctionNode);
            StatementNodeVector& vA =((SwitchStatementNode *)currentNode)->m_casesA;
            for (unsigned i = 0; i < vA.size() ; i ++)
                postAnalysisFunctionForCalcID(vA[i], nearFunctionNode);
            postAnalysisFunctionForCalcID(((SwitchStatementNode *)currentNode)->m_default, nearFunctionNode);
            StatementNodeVector& vB = ((SwitchStatementNode *)currentNode)->m_casesB;
            for (unsigned i = 0; i < vB.size() ; i ++)
                postAnalysisFunctionForCalcID(vB[i], nearFunctionNode);
        } else if (type == NodeType::SwitchCase) {
            postAnalysisFunctionForCalcID(((SwitchCaseNode *)currentNode)->m_test, nearFunctionNode);
            StatementNodeVector& v = ((SwitchCaseNode *)currentNode)->m_consequent;
            for (unsigned i = 0; i < v.size() ; i ++)
                postAnalysisFunctionForCalcID(v[i], nearFunctionNode);
        } else if (type == NodeType::ThisExpression) {

        } else if (type == NodeType::BreakStatement) {
        } else if (type == NodeType::ContinueStatement) {
        } else if (type == NodeType::ReturnStatement) {
            postAnalysisFunctionForCalcID(((ReturnStatmentNode *)currentNode)->m_argument, nearFunctionNode);
        } else if (type == NodeType::EmptyStatement) {
        } else if (type == NodeType::TryStatement) {
            postAnalysisFunctionForCalcID(((TryStatementNode *)currentNode)->m_block, nearFunctionNode);
            postAnalysisFunctionForCalcID(((TryStatementNode *)currentNode)->m_handler, nearFunctionNode);
            postAnalysisFunctionForCalcID(((TryStatementNode *)currentNode)->m_finalizer, nearFunctionNode);
        } else if (type == NodeType::CatchClause) {
            postAnalysisFunctionForCalcID(((CatchClauseNode *)currentNode)->m_param, nearFunctionNode);
            postAnalysisFunctionForCalcID(((CatchClauseNode *)currentNode)->m_guard, nearFunctionNode);
            postAnalysisFunctionForCalcID(((CatchClauseNode *)currentNode)->m_body, nearFunctionNode);
        } else if (type == NodeType::ThrowStatement) {
            postAnalysisFunctionForCalcID(((ThrowStatementNode *)currentNode)->m_argument, nearFunctionNode);
        } else if (type == NodeType::LabeledStatement) {
            postAnalysisFunctionForCalcID(((LabeledStatementNode *)currentNode)->m_statementNode, nearFunctionNode);
        } else if (type == NodeType::BreakLabelStatement) {
        } else if (type == NodeType::ContinueLabelStatement) {
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    };

    postAnalysisFunctionForCalcID(programNode, NULL);
}

CodeBlock* ScriptParser::parseScript(ESVMInstance* instance, escargot::ESString* source, ExecutableType type, ParserContextInformation& parserContextInformation)
{
#ifdef ENABLE_CODECACHE
    if (source->length() < options::CodeCacheThreshold) {
        if (parserContextInformation.m_isForGlobalScope) {
            auto iter = m_globalCodeCache.find(std::make_pair(source, parserContextInformation.hash()));
            if (iter != m_globalCodeCache.end()) {
                return iter->second;
            }
        } else {
            auto iter = m_nonGlobalCodeCache.find(std::make_pair(source, parserContextInformation.hash()));
            if (iter != m_nonGlobalCodeCache.end()) {
                return iter->second;
            }
        }
    }
#endif
    // unsigned long start = ESVMInstance::currentInstance()->tickCount();

    ProgramNode* programNode;
    try {
        programNode = esprima::parse(source, parserContextInformation.m_strictFromOutside);

        // unsigned long end = ESVMInstance::currentInstance()->tickCount();
        // ESCARGOT_LOG_ERROR("parse takes %lfms\n", (end-start)/1000.0);
        // printf("esprima takes %lfms\n", (end-start)/1000.0);
    } catch(const EsprimaError& error) {
        char temp[512];
        if (type != ExecutableType::EvalCode)
            snprintf(temp, sizeof(temp), "%s (Parse Error %zu line)", error.m_message->utf8Data(), error.m_lineNumber);
        else
            snprintf(temp, sizeof(temp), "%s", error.m_message->utf8Data());
        ESVMInstance::currentInstance()->throwError(ESErrorObject::create(ESString::create(temp), error.m_code));
    }

    if (type == ExecutableType::EvalCode && programNode->isStrict()) {
        parserContextInformation.m_isForGlobalScope = false;
    }
    analyzeAST(instance, parserContextInformation, programNode);
    CodeBlock* cb = generateByteCode(nullptr, programNode, type, parserContextInformation, source->length() > options::LazyByteCodeGenerationThreshold ? false : true);
    // unsigned long end = ESVMInstance::currentInstance()->tickCount();
    // printf("parseScript takes %lfms\n", (end-start)/1000.0);

#ifdef ENABLE_CODECACHE
    if (source->length() < options::CodeCacheThreshold) {
        if (parserContextInformation.m_isForGlobalScope) {
            cb->m_isCached = true;
            m_globalCodeCache.insert(std::make_pair(std::make_pair(source, parserContextInformation.hash()), cb));
        } else {
            cb->m_isCached = true;
            m_nonGlobalCodeCache.insert(std::make_pair(std::make_pair(source, parserContextInformation.hash()), cb));
        }
    }
#endif

    return cb;
}

CodeBlock* ScriptParser::parseSingleFunction(ESVMInstance* instance, escargot::ESString* argSource, escargot::ESString* bodySource, ParserContextInformation& parserContextInformation)
{
    // unsigned long start = ESVMInstance::currentInstance()->tickCount();

    ProgramNode* programNode;

    try {
        programNode = esprima::parseSingleFunction(argSource, bodySource);
        ASSERT(programNode->body()[1]->type() == escargot::NodeType::FunctionDeclaration);
    } catch(const EsprimaError& error) {
        char temp[512];
        snprintf(temp, sizeof(temp), "%s", error.m_message->utf8Data());
        ESVMInstance::currentInstance()->throwError(ESErrorObject::create(ESString::create(temp), error.m_code));
    }

    analyzeAST(instance, parserContextInformation, programNode);
    FunctionNode* functionDeclAST = static_cast<FunctionNode* >(programNode->body()[1]);
    CodeBlock* codeBlock = generateByteCode(nullptr, functionDeclAST, ExecutableType::FunctionCode, parserContextInformation, true);

    // unsigned long end = ESVMInstance::currentInstance()->tickCount();
    // printf("parseScript takes %lfms\n", (end-start)/1000.0);
    return codeBlock;

}

}
