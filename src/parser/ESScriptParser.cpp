#include "Escargot.h"
#include "ESScriptParser.h"
#include "vm/ESVMInstance.h"
#include "runtime/ESValue.h"

#include "esprima.h"

#ifdef ESCARGOT_PROFILE
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#endif

namespace escargot {

unsigned long getLongTickCount()
{
    struct timespec timespec;
    clock_gettime(CLOCK_MONOTONIC,&timespec);
    return (unsigned long)(timespec.tv_sec * 1000000L + timespec.tv_nsec/1000);
}

#ifdef ESCARGOT_PROFILE
void ESScriptParser::dumpStats()
{
    unsigned stat;
    auto stream = stderr;

    stat = JS_GetGCParameter(s_rt, JSGC_TOTAL_CHUNKS);
    fwprintf(stream, L"[MOZJS] JSGC_TOTAL_CHUNKS: %d\n", stat);
    stat = JS_GetGCParameter(s_rt, JSGC_UNUSED_CHUNKS);
    fwprintf(stream, L"[MOZJS] JSGC_UNUSED_CHUNKS: %d\n", stat);

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
    if(stat > 10000) {
        while(true) {}
    }
#endif
}
#endif

Node* ESScriptParser::parseScript(ESVMInstance* instance, const escargot::u16string& source)
{
    Node* node;
    try {
        node = esprima::parse(source);
    } catch(...) {
        throw SyntaxError();
    }

    auto markNeedsActivation = [](FunctionNode* nearFunctionNode){
        FunctionNode* node = nearFunctionNode;
        while(node) {
            node->setNeedsActivation(true);
            node = node->outerFunctionNode();
        }
    };

    std::function<void (Node* currentNode, std::vector<InternalAtomicStringVector *>& identifierStack,
            FunctionNode* nearFunctionNode)> postAnalysisFunction =
            [&postAnalysisFunction, instance, &markNeedsActivation](Node* currentNode, std::vector<InternalAtomicStringVector *>& identifierStack,
                    FunctionNode* nearFunctionNode) {
        if(!currentNode)
            return;
        NodeType type = currentNode->type();
        InternalAtomicStringVector& identifierInCurrentContext = *identifierStack.back();
        if(type == NodeType::Program) {
            StatementNodeVector& v = ((ProgramNode *)currentNode)->m_body;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
            }
        } else if(type == NodeType::VariableDeclaration) {
            VariableDeclaratorVector& v = ((VariableDeclarationNode *)currentNode)->m_declarations;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
            }
        } else if(type == NodeType::VariableDeclarator) {
            //wprintf(L"add Identifier %ls(var)\n", ((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name().data());
            if(identifierInCurrentContext.end() == std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),
                    ((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name())) {
                identifierInCurrentContext.push_back(((IdentifierNode *)((VariableDeclaratorNode *)currentNode)->m_id)->name());
            }
        } else if(type == NodeType::FunctionDeclaration) {
            //TODO
            //wprintf(L"add Identifier %ls(fn)\n", ((FunctionDeclarationNode *)currentNode)->id().data());
            if(identifierInCurrentContext.end() == std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),
                    ((FunctionDeclarationNode *)currentNode)->id())) {
                identifierInCurrentContext.push_back(((FunctionDeclarationNode *)currentNode)->id());
            }
            //wprintf(L"process function body-------------------\n");
            InternalAtomicStringVector newIdentifierVector;
            InternalAtomicStringVector& vec = ((FunctionExpressionNode *)currentNode)->m_params;
            for(unsigned i = 0; i < vec.size() ; i ++) {
                if(newIdentifierVector.end() == std::find(newIdentifierVector.begin(),newIdentifierVector.end(),
                        vec[i])) {
                    newIdentifierVector.push_back(vec[i]);
                }
            }
            ((FunctionDeclarationNode *)currentNode)->setOuterFunctionNode(nearFunctionNode);
            identifierStack.push_back(&newIdentifierVector);
            postAnalysisFunction(((FunctionDeclarationNode *)currentNode)->m_body, identifierStack, ((FunctionDeclarationNode *)currentNode));
            identifierStack.pop_back();
            ((FunctionDeclarationNode *)currentNode)->setInnerIdentifiers(std::move(newIdentifierVector));
            //wprintf(L"end of process function body-------------------\n");
        } else if(type == NodeType::FunctionExpression) {
            //wprintf(L"process function body-------------------\n");
            InternalAtomicStringVector newIdentifierVector;
            InternalAtomicStringVector& vec = ((FunctionExpressionNode *)currentNode)->m_params;
            for(unsigned i = 0; i < vec.size() ; i ++) {
                if(newIdentifierVector.end() == std::find(newIdentifierVector.begin(),newIdentifierVector.end(),
                        vec[i])) {
                    newIdentifierVector.push_back(vec[i]);
                }
            }
            ((FunctionExpressionNode *)currentNode)->setOuterFunctionNode(nearFunctionNode);
            identifierStack.push_back(&newIdentifierVector);
            postAnalysisFunction(((FunctionExpressionNode *)currentNode)->m_body, identifierStack, ((FunctionExpressionNode *)currentNode));
            identifierStack.pop_back();
            ((FunctionExpressionNode *)currentNode)->setInnerIdentifiers(std::move(newIdentifierVector));
            //wprintf(L"end of process function body-------------------\n");
        } else if(type == NodeType::Identifier) {
            //use case
            InternalAtomicString name = ((IdentifierNode *)currentNode)->name();
            //ESString* nonAtomicName = ((IdentifierNode *)currentNode)->nonAtomicName();
            auto iter = std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),name);
            if(name == strings->atomicArguments && iter == identifierInCurrentContext.end() && nearFunctionNode) {
                identifierInCurrentContext.push_back(strings->atomicArguments);
                nearFunctionNode->markNeedsArgumentsObject();
                iter = std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),name);
            }
            if(identifierInCurrentContext.end() == iter) {
                //search top...
                unsigned up = 0;
                for(int i = identifierStack.size() - 2 ; i >= 0 ; i --) {
                    up++;
                    InternalAtomicStringVector* vector = identifierStack[i];
                    auto iter2 = std::find(vector->begin(),vector->end(),name);
                    if(iter2 != vector->end()) {
                        FunctionNode* fn = nearFunctionNode;
                        for(unsigned j = 0; j < up ; j ++) {
                            fn = fn->outerFunctionNode();
                        }
                        if(fn) {
                            //printf("outer function of this function  needs capture! -> because fn...%s iden..%s\n",
                            //        fn->nonAtomicId()->utf8Data(),
                            //        ((IdentifierNode *)currentNode)->nonAtomicName()->utf8Data());
                            markNeedsActivation(fn);
                        } else {
                            //fn == global case
                        }
                        break;
                    }
                }

                /*
                if(!instance->globalObject()->hasKey(nonAtomicName)) {
                    if(nearFunctionNode && nearFunctionNode->outerFunctionNode()) {
                        wprintf(L"outer function of this function  needs capture! -> because %ls\n", ((IdentifierNode *)currentNode)->name().data());
                        markNeedsActivation(nearFunctionNode->outerFunctionNode());
                    }
                }*/
            } else {
                if(nearFunctionNode) {
                    size_t idx = std::distance(identifierInCurrentContext.begin(), iter);
                    ((IdentifierNode *)currentNode)->setFastAccessIndex(idx);
                }
            }
            //wprintf(L"use Identifier %ls\n", ((IdentifierNode *)currentNode)->name().data());
        } else if(type == NodeType::ExpressionStatement) {
            postAnalysisFunction(((ExpressionStatementNode *)currentNode)->m_expression, identifierStack, nearFunctionNode);
        } else if(type == NodeType::AssignmentExpression) {
            postAnalysisFunction(((AssignmentExpressionNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
            postAnalysisFunction(((AssignmentExpressionNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
        } else if(type == NodeType::AssignmentExpressionSimple) {
            postAnalysisFunction(((AssignmentExpressionSimpleNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
            postAnalysisFunction(((AssignmentExpressionSimpleNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
        } else if(type == NodeType::Literal) {
            //DO NOTHING
        }else if(type == NodeType::ArrayExpression) {
            ExpressionNodeVector& v = ((ArrayExpressionNode *)currentNode)->m_elements;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
            }
        } else if(type == NodeType::BlockStatement) {
            StatementNodeVector& v = ((BlockStatementNode *)currentNode)->m_body;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
            }
        } else if(type == NodeType::CallExpression) {

            Node * callee = ((CallExpressionNode *)currentNode)->m_callee;
            if(callee) {
                if(callee->type() == NodeType::Identifier) {
                    if(((IdentifierNode *)callee)->name() == InternalAtomicString(u"eval")) {
                        markNeedsActivation(nearFunctionNode);
                    }
                }
            }

            postAnalysisFunction(callee, identifierStack, nearFunctionNode);
            ArgumentVector& v = ((CallExpressionNode *)currentNode)->m_arguments;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
            }
        } else if(type == NodeType::SequenceExpression) {
            ExpressionNodeVector& v = ((SequenceExpressionNode *)currentNode)->m_expressions;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
            }
        } else if(type == NodeType::NewExpression) {
            postAnalysisFunction(((NewExpressionNode *)currentNode)->m_callee, identifierStack, nearFunctionNode);
            ArgumentVector& v = ((NewExpressionNode *)currentNode)->m_arguments;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
            }
        } else if(type == NodeType::ObjectExpression) {
            PropertiesNodeVector& v = ((ObjectExpressionNode *)currentNode)->m_properties;
            for(unsigned i = 0; i < v.size() ; i ++) {
                PropertyNode* p = v[i];
                postAnalysisFunction(p->value(), identifierStack, nearFunctionNode);
                if(p->key()->type() == NodeType::Identifier) {

                } else {
                    postAnalysisFunction(p->key(), identifierStack, nearFunctionNode);
                }
            }
        } else if(type == NodeType::ConditionalExpression) {
            postAnalysisFunction(((ConditionalExpressionNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
            postAnalysisFunction(((ConditionalExpressionNode *)currentNode)->m_consequente, identifierStack, nearFunctionNode);
            postAnalysisFunction(((ConditionalExpressionNode *)currentNode)->m_alternate, identifierStack, nearFunctionNode);
        } else if(type == NodeType::Property) {
            postAnalysisFunction(((PropertyNode *)currentNode)->m_key, identifierStack, nearFunctionNode);
            postAnalysisFunction(((PropertyNode *)currentNode)->m_value, identifierStack, nearFunctionNode);
        } else if(type == NodeType::MemberExpression) {
            postAnalysisFunction(((MemberExpressionNode *)currentNode)->m_object, identifierStack, nearFunctionNode);
            postAnalysisFunction(((MemberExpressionNode *)currentNode)->m_property, identifierStack, nearFunctionNode);
        } else if(type == NodeType::MemberExpressionNonComputedCase) {
            postAnalysisFunction(((MemberExpressionNodeNonComputedCase *)currentNode)->m_object, identifierStack, nearFunctionNode);
        } else if(type == NodeType::BinaryExpression) {
            postAnalysisFunction(((BinaryExpressionNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
            postAnalysisFunction(((BinaryExpressionNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
        } else if(type >= NodeType::BinaryExpressionBitwiseAnd && type <= NodeType::BinaryExpressionUnsignedRightShift) {
            postAnalysisFunction(((BinaryExpressionBitwiseAndNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
            postAnalysisFunction(((BinaryExpressionBitwiseAndNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
        } else if(type == NodeType::LogicalExpression) {
            postAnalysisFunction(((LogicalExpressionNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
            postAnalysisFunction(((LogicalExpressionNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
        } else if(type >= NodeType::UpdateExpressionDecrementPostfix && type <= UpdateExpressionIncrementPrefix) {
            postAnalysisFunction(((UpdateExpressionDecrementPostfixNode *)currentNode)->m_argument, identifierStack, nearFunctionNode);
        } else if(type >= NodeType::UnaryExpressionBitwiseNot && type <= NodeType::UnaryExpressionTypeOf) {
            postAnalysisFunction(((UnaryExpressionBitwiseNotNode *)currentNode)->m_argument, identifierStack, nearFunctionNode);
        } else if(type == NodeType::IfStatement) {
            postAnalysisFunction(((IfStatementNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
            postAnalysisFunction(((IfStatementNode *)currentNode)->m_consequente, identifierStack, nearFunctionNode);
            postAnalysisFunction(((IfStatementNode *)currentNode)->m_alternate, identifierStack, nearFunctionNode);
        } else if(type == NodeType::ForStatement) {
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_init, identifierStack, nearFunctionNode);
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_body, identifierStack, nearFunctionNode);
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_update, identifierStack, nearFunctionNode);
        } else if(type == NodeType::ForInStatement) {
            postAnalysisFunction(((ForInStatementNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
            postAnalysisFunction(((ForInStatementNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
            postAnalysisFunction(((ForInStatementNode *)currentNode)->m_body, identifierStack, nearFunctionNode);
        } else if(type == NodeType::WhileStatement) {
            postAnalysisFunction(((WhileStatementNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
            postAnalysisFunction(((WhileStatementNode *)currentNode)->m_body, identifierStack, nearFunctionNode);
        } else if(type == NodeType::DoWhileStatement) {
            postAnalysisFunction(((DoWhileStatementNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
            postAnalysisFunction(((DoWhileStatementNode *)currentNode)->m_body, identifierStack, nearFunctionNode);
        } else if(type == NodeType::SwitchStatement) {
            postAnalysisFunction(((SwitchStatementNode *)currentNode)->m_discriminant, identifierStack, nearFunctionNode);
            StatementNodeVector& vA =((SwitchStatementNode *)currentNode)->m_casesA;
            for(unsigned i = 0; i < vA.size() ; i ++)
                postAnalysisFunction(vA[i], identifierStack, nearFunctionNode);
            postAnalysisFunction(((SwitchStatementNode *)currentNode)->m_default, identifierStack, nearFunctionNode);
            StatementNodeVector& vB = ((SwitchStatementNode *)currentNode)->m_casesB;
            for(unsigned i = 0; i < vB.size() ; i ++)
                postAnalysisFunction(vB[i], identifierStack, nearFunctionNode);
        } else if(type == NodeType::SwitchCase) {
            postAnalysisFunction(((SwitchCaseNode *)currentNode)->m_test, identifierStack, nearFunctionNode);
            StatementNodeVector& v = ((SwitchCaseNode *)currentNode)->m_consequent;
            for(unsigned i = 0; i < v.size() ; i ++)
                postAnalysisFunction(v[i], identifierStack, nearFunctionNode);
        } else if(type == NodeType::ThisExpression) {

        } else if(type == NodeType::BreakStatement) {

        } else if(type == NodeType::ContinueStatement) {

        } else if(type == NodeType::ReturnStatement) {
            postAnalysisFunction(((ReturnStatmentNode *)currentNode)->m_argument, identifierStack, nearFunctionNode);
        } else if(type == NodeType::EmptyStatement) {
        } else if (type == NodeType::TryStatement) {
            postAnalysisFunction(((TryStatementNode *)currentNode)->m_block, identifierStack, nearFunctionNode);
            postAnalysisFunction(((TryStatementNode *)currentNode)->m_handler, identifierStack, nearFunctionNode);
            postAnalysisFunction(((TryStatementNode *)currentNode)->m_finalizer, identifierStack, nearFunctionNode);
        } else if (type == NodeType::CatchClause) {
            markNeedsActivation(nearFunctionNode);
            postAnalysisFunction(((CatchClauseNode *)currentNode)->m_param, identifierStack, nearFunctionNode);
            postAnalysisFunction(((CatchClauseNode *)currentNode)->m_guard, identifierStack, nearFunctionNode);
            postAnalysisFunction(((CatchClauseNode *)currentNode)->m_body, identifierStack, nearFunctionNode);
        } else if (type == NodeType::ThrowStatement) {
            postAnalysisFunction(((ThrowStatementNode *)currentNode)->m_argument, identifierStack, nearFunctionNode);
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    };

    InternalAtomicStringVector identifierInCurrentContext;
    std::vector<InternalAtomicStringVector *> stack;
    stack.push_back(&identifierInCurrentContext);
    postAnalysisFunction(node, stack, NULL);

    std::function<void (Node** node, FunctionNode* nearFunction)> nodeReplacer = [](Node** node, FunctionNode* nearFunction) {
        if(*node) {
            if((*node)->type() == NodeType::Identifier) {
                IdentifierNode* n = (IdentifierNode *)*node;
                if(nearFunction && !nearFunction->needsActivation() && n->canUseFastAccess()) {
#ifdef NDEBUG
                    *node = new IdentifierFastCaseNode(n->fastAccessIndex());
#else
                    *node = new IdentifierFastCaseNode(n->fastAccessIndex(), n->name());
#endif
                }
            }
        }
    };

    std::function<void (Node* currentNode, FunctionNode* nearFunction)> postProcessingFunction =
            [&postProcessingFunction, &nodeReplacer](Node* currentNode, FunctionNode* nearFunction) {
        if(!currentNode)
            return;
        NodeType type = currentNode->type();
        if(type == NodeType::Program) {
            StatementNodeVector& v = ((ProgramNode *)currentNode)->m_body;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postProcessingFunction(v[i], nearFunction);
            }
        } else if(type == NodeType::VariableDeclaration) {
            VariableDeclaratorVector& v = ((VariableDeclarationNode *)currentNode)->m_declarations;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postProcessingFunction(v[i], nearFunction);
            }
        } else if(type == NodeType::VariableDeclarator) {
        } else if(type == NodeType::FunctionDeclaration) {
            postProcessingFunction(((FunctionDeclarationNode *)currentNode)->m_body, (FunctionDeclarationNode *)currentNode);
        } else if(type == NodeType::FunctionExpression) {
            postProcessingFunction(((FunctionExpressionNode *)currentNode)->m_body, (FunctionExpressionNode *)currentNode);
        } else if(type == NodeType::Identifier) {
        } else if(type == NodeType::ExpressionStatement) {
            nodeReplacer(&((ExpressionStatementNode *)currentNode)->m_expression, nearFunction);
            postProcessingFunction(((ExpressionStatementNode *)currentNode)->m_expression, nearFunction);
        } else if(type == NodeType::AssignmentExpression) {
            nodeReplacer(&((AssignmentExpressionNode *)currentNode)->m_right, nearFunction);
            nodeReplacer(&((AssignmentExpressionNode *)currentNode)->m_left, nearFunction);
            postProcessingFunction(((AssignmentExpressionNode *)currentNode)->m_right, nearFunction);
            postProcessingFunction(((AssignmentExpressionNode *)currentNode)->m_left, nearFunction);
        } else if(type == NodeType::AssignmentExpressionSimple) {
            nodeReplacer(&((AssignmentExpressionSimpleNode*)currentNode)->m_right, nearFunction);
            nodeReplacer(&((AssignmentExpressionSimpleNode *)currentNode)->m_left, nearFunction);
            postProcessingFunction(((AssignmentExpressionSimpleNode *)currentNode)->m_right, nearFunction);
            postProcessingFunction(((AssignmentExpressionSimpleNode *)currentNode)->m_left, nearFunction);
        } else if(type == NodeType::Literal) {
            //DO NOTHING
        }else if(type == NodeType::ArrayExpression) {
            ExpressionNodeVector& v = ((ArrayExpressionNode *)currentNode)->m_elements;
            for(unsigned i = 0; i < v.size() ; i ++) {
                nodeReplacer(&v[i], nearFunction);
                postProcessingFunction(v[i], nearFunction);
            }
        } else if(type == NodeType::BlockStatement) {
            StatementNodeVector& v = ((BlockStatementNode *)currentNode)->m_body;
            for(unsigned i = 0; i < v.size() ; i ++) {
                nodeReplacer(&v[i], nearFunction);
                postProcessingFunction(v[i], nearFunction);
            }
        } else if(type == NodeType::CallExpression) {
            Node * callee = ((CallExpressionNode *)currentNode)->m_callee;
            nodeReplacer(&((CallExpressionNode *)currentNode)->m_callee, nearFunction);
            postProcessingFunction(callee, nearFunction);
            ArgumentVector& v = ((CallExpressionNode *)currentNode)->m_arguments;
            for(unsigned i = 0; i < v.size() ; i ++) {
                nodeReplacer(&v[i], nearFunction);
                postProcessingFunction(v[i], nearFunction);
            }
        } else if(type == NodeType::SequenceExpression) {
            ExpressionNodeVector& v = ((SequenceExpressionNode *)currentNode)->m_expressions;
            for(unsigned i = 0; i < v.size() ; i ++) {
                nodeReplacer(&v[i], nearFunction);
                postProcessingFunction(v[i], nearFunction);
            }
        } else if(type == NodeType::NewExpression) {
            postProcessingFunction(((NewExpressionNode *)currentNode)->m_callee, nearFunction);
            ArgumentVector& v = ((NewExpressionNode *)currentNode)->m_arguments;
            for(unsigned i = 0; i < v.size() ; i ++) {
                nodeReplacer(&v[i], nearFunction);
                postProcessingFunction(v[i], nearFunction);
            }
        } else if(type == NodeType::ObjectExpression) {
            PropertiesNodeVector& v = ((ObjectExpressionNode *)currentNode)->m_properties;
            for(unsigned i = 0; i < v.size() ; i ++) {
                PropertyNode* p = v[i];
                postProcessingFunction(p->value(), nearFunction);
                if(p->key()->type() == NodeType::Identifier) {

                } else {
                    nodeReplacer(&p->m_key, nearFunction);
                    postProcessingFunction(p->key(), nearFunction);
                }
            }
        } else if(type == NodeType::ConditionalExpression) {
            nodeReplacer((Node **)&((ConditionalExpressionNode *)currentNode)->m_test, nearFunction);
            nodeReplacer((Node **)&((ConditionalExpressionNode *)currentNode)->m_consequente, nearFunction);
            nodeReplacer((Node **)&((ConditionalExpressionNode *)currentNode)->m_alternate, nearFunction);
            postProcessingFunction(((ConditionalExpressionNode *)currentNode)->m_test, nearFunction);
            postProcessingFunction(((ConditionalExpressionNode *)currentNode)->m_consequente, nearFunction);
            postProcessingFunction(((ConditionalExpressionNode *)currentNode)->m_alternate, nearFunction);
        } else if(type == NodeType::Property) {
            postProcessingFunction(((PropertyNode *)currentNode)->m_key, nearFunction);
            postProcessingFunction(((PropertyNode *)currentNode)->m_value, nearFunction);
        } else if(type == NodeType::MemberExpression) {
            nodeReplacer(&((MemberExpressionNode *)currentNode)->m_object, nearFunction);
            nodeReplacer(&((MemberExpressionNode *)currentNode)->m_property, nearFunction);
            postProcessingFunction(((MemberExpressionNode *)currentNode)->m_object, nearFunction);
            postProcessingFunction(((MemberExpressionNode *)currentNode)->m_property, nearFunction);
        } else if(type == NodeType::MemberExpressionNonComputedCase) {
            nodeReplacer(&((MemberExpressionNodeNonComputedCase *)currentNode)->m_object, nearFunction);
            postProcessingFunction(((MemberExpressionNodeNonComputedCase *)currentNode)->m_object, nearFunction);
        } else if(type == NodeType::BinaryExpression) {
            nodeReplacer((Node **)&((BinaryExpressionNode *)currentNode)->m_right, nearFunction);
            nodeReplacer((Node **)&((BinaryExpressionNode *)currentNode)->m_left, nearFunction);
            postProcessingFunction(((BinaryExpressionNode *)currentNode)->m_right, nearFunction);
            postProcessingFunction(((BinaryExpressionNode *)currentNode)->m_left, nearFunction);
        } else if(type >= NodeType::BinaryExpressionBitwiseAnd && type <= NodeType::BinaryExpressionUnsignedRightShift) {
            nodeReplacer((Node **)&((BinaryExpressionBitwiseAndNode *)currentNode)->m_right, nearFunction);
            nodeReplacer((Node **)&((BinaryExpressionBitwiseAndNode *)currentNode)->m_left, nearFunction);
            postProcessingFunction(((BinaryExpressionBitwiseAndNode *)currentNode)->m_right, nearFunction);
            postProcessingFunction(((BinaryExpressionBitwiseAndNode *)currentNode)->m_left, nearFunction);
        } else if(type == NodeType::LogicalExpression) {
            nodeReplacer((Node **)&((LogicalExpressionNode *)currentNode)->m_right, nearFunction);
            nodeReplacer((Node **)&((LogicalExpressionNode *)currentNode)->m_left, nearFunction);
            postProcessingFunction(((LogicalExpressionNode *)currentNode)->m_right, nearFunction);
            postProcessingFunction(((LogicalExpressionNode *)currentNode)->m_left, nearFunction);
        } else if(type >= NodeType::UpdateExpressionDecrementPostfix && type <= UpdateExpressionIncrementPrefix) {
            nodeReplacer((Node **)&((UpdateExpressionDecrementPostfixNode *)currentNode)->m_argument, nearFunction);
            postProcessingFunction(((UpdateExpressionDecrementPostfixNode *)currentNode)->m_argument, nearFunction);
        } else if(type >= NodeType::UnaryExpressionBitwiseNot && type <= NodeType::UnaryExpressionTypeOf) {
            nodeReplacer(&((UnaryExpressionBitwiseNotNode *)currentNode)->m_argument, nearFunction);
            postProcessingFunction(((UnaryExpressionBitwiseNotNode *)currentNode)->m_argument, nearFunction);
        } else if(type == NodeType::IfStatement) {
            nodeReplacer((Node **)&((IfStatementNode *)currentNode)->m_test, nearFunction);
            nodeReplacer((Node **)&((IfStatementNode *)currentNode)->m_consequente, nearFunction);
            nodeReplacer((Node **)&((IfStatementNode *)currentNode)->m_alternate, nearFunction);
            postProcessingFunction(((IfStatementNode *)currentNode)->m_test, nearFunction);
            postProcessingFunction(((IfStatementNode *)currentNode)->m_consequente, nearFunction);
            postProcessingFunction(((IfStatementNode *)currentNode)->m_alternate, nearFunction);
        } else if(type == NodeType::ForStatement) {
            nodeReplacer((Node **)&((ForStatementNode *)currentNode)->m_init, nearFunction);
            nodeReplacer((Node **)&((ForStatementNode *)currentNode)->m_body, nearFunction);
            nodeReplacer((Node **)&((ForStatementNode *)currentNode)->m_test, nearFunction);
            nodeReplacer((Node **)&((ForStatementNode *)currentNode)->m_update, nearFunction);
            postProcessingFunction(((ForStatementNode *)currentNode)->m_init, nearFunction);
            postProcessingFunction(((ForStatementNode *)currentNode)->m_body, nearFunction);
            postProcessingFunction(((ForStatementNode *)currentNode)->m_test, nearFunction);
            postProcessingFunction(((ForStatementNode *)currentNode)->m_update, nearFunction);
        } else if(type == NodeType::ForInStatement) {
            nodeReplacer((Node **)&((ForInStatementNode *)currentNode)->m_left, nearFunction);
            nodeReplacer((Node **)&((ForInStatementNode *)currentNode)->m_right, nearFunction);
            nodeReplacer((Node **)&((ForInStatementNode *)currentNode)->m_body, nearFunction);
            postProcessingFunction(((ForInStatementNode *)currentNode)->m_left, nearFunction);
            postProcessingFunction(((ForInStatementNode *)currentNode)->m_right, nearFunction);
            postProcessingFunction(((ForInStatementNode *)currentNode)->m_body, nearFunction);
        } else if(type == NodeType::WhileStatement) {
            nodeReplacer((Node **)&((WhileStatementNode *)currentNode)->m_test, nearFunction);
            nodeReplacer((Node **)&((WhileStatementNode *)currentNode)->m_body, nearFunction);
            postProcessingFunction(((WhileStatementNode *)currentNode)->m_test, nearFunction);
            postProcessingFunction(((WhileStatementNode *)currentNode)->m_body, nearFunction);
        } else if(type == NodeType::DoWhileStatement) {
            nodeReplacer((Node **)&((DoWhileStatementNode *)currentNode)->m_test, nearFunction);
            nodeReplacer((Node **)&((DoWhileStatementNode *)currentNode)->m_body, nearFunction);
            postProcessingFunction(((DoWhileStatementNode *)currentNode)->m_test, nearFunction);
            postProcessingFunction(((DoWhileStatementNode *)currentNode)->m_body, nearFunction);
        } else if(type == NodeType::SwitchStatement) {
            nodeReplacer((Node **)&((SwitchStatementNode *)currentNode)->m_discriminant, nearFunction);
            postProcessingFunction(((SwitchStatementNode *)currentNode)->m_discriminant, nearFunction);
            StatementNodeVector& vA =((SwitchStatementNode *)currentNode)->m_casesA;
            for(unsigned i = 0; i < vA.size() ; i ++) {
                nodeReplacer((Node **)&vA[i], nearFunction);
                postProcessingFunction(vA[i], nearFunction);
            }
            nodeReplacer((Node **)&((SwitchStatementNode *)currentNode)->m_default, nearFunction);
            postProcessingFunction(((SwitchStatementNode *)currentNode)->m_default, nearFunction);
            StatementNodeVector& vB = ((SwitchStatementNode *)currentNode)->m_casesB;
            for(unsigned i = 0; i < vB.size() ; i ++) {
                nodeReplacer((Node **)&vB[i], nearFunction);
                postProcessingFunction(vB[i], nearFunction);
            }
        } else if(type == NodeType::SwitchCase) {
            nodeReplacer((Node **)&((SwitchCaseNode *)currentNode)->m_test, nearFunction);
            postProcessingFunction(((SwitchCaseNode *)currentNode)->m_test, nearFunction);
            StatementNodeVector& v = ((SwitchCaseNode *)currentNode)->m_consequent;
            for(unsigned i = 0; i < v.size() ; i ++) {
                nodeReplacer((Node **)&v[i], nearFunction);
                postProcessingFunction(v[i], nearFunction);
            }
        } else if(type == NodeType::ThisExpression) {

        } else if(type == NodeType::BreakStatement) {

        } else if(type == NodeType::ContinueStatement) {

        } else if(type == NodeType::ReturnStatement) {
            nodeReplacer((Node **)&((ReturnStatmentNode *)currentNode)->m_argument, nearFunction);
            postProcessingFunction(((ReturnStatmentNode *)currentNode)->m_argument, nearFunction);
        } else if(type == NodeType::EmptyStatement) {
        } else if (type == NodeType::TryStatement) {
            nodeReplacer((Node **)&((TryStatementNode *)currentNode)->m_block, nearFunction);
            nodeReplacer((Node **)&((TryStatementNode *)currentNode)->m_handler, nearFunction);
            nodeReplacer((Node **)&((TryStatementNode *)currentNode)->m_finalizer, nearFunction);
            postProcessingFunction(((TryStatementNode *)currentNode)->m_block, nearFunction);
            postProcessingFunction(((TryStatementNode *)currentNode)->m_handler, nearFunction);
            postProcessingFunction(((TryStatementNode *)currentNode)->m_finalizer, nearFunction);
        } else if (type == NodeType::CatchClause) {
            postProcessingFunction(((CatchClauseNode *)currentNode)->m_param, nearFunction);
            postProcessingFunction(((CatchClauseNode *)currentNode)->m_guard, nearFunction);
            postProcessingFunction(((CatchClauseNode *)currentNode)->m_body, nearFunction);
        } else if (type == NodeType::ThrowStatement) {
            nodeReplacer((Node **)&((ThrowStatementNode *)currentNode)->m_argument, nearFunction);
            postProcessingFunction(((ThrowStatementNode *)currentNode)->m_argument, nearFunction);
        } else if(type == NodeType::IdentifierFastCase) {

        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    };

    postProcessingFunction(node, NULL);

    return node;
}

}
