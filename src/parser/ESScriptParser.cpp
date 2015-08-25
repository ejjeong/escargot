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

ESString*  astTypeProgram = ESString::create(u"Program");
ESString*  astTypeVariableDeclaration = ESString::create(u"VariableDeclaration");
ESString*  astTypeExpressionStatement = ESString::create(u"ExpressionStatement");
ESString*  astTypeVariableDeclarator = ESString::create(u"VariableDeclarator");
ESString*  astTypeIdentifier = ESString::create(u"Identifier");
ESString*  astTypeAssignmentExpression = ESString::create(u"AssignmentExpression");
ESString*  astTypeThisExpression = ESString::create(u"ThisExpression");
ESString*  astTypeBreakStatement = ESString::create(u"BreakStatement");
ESString*  astTypeContinueStatement = ESString::create(u"ContinueStatement");
ESString*  astTypeReturnStatement = ESString::create(u"ReturnStatement");
ESString*  astTypeEmptyStatement = ESString::create(u"EmptyStatement");
ESString*  astTypeLiteral = ESString::create(u"Literal");
ESString*  astTypeFunctionDeclaration = ESString::create(u"FunctionDeclaration");
ESString*  astTypeFunctionExpression = ESString::create(u"FunctionExpression");
ESString*  astTypeBlockStatement = ESString::create(u"BlockStatement");
ESString*  astTypeArrayExpression = ESString::create(u"ArrayExpression");
ESString*  astTypeCallExpression = ESString::create(u"CallExpression");
ESString*  astTypeObjectExpression = ESString::create(u"ObjectExpression");
ESString*  astTypeMemberExpression = ESString::create(u"MemberExpression");
ESString*  astTypeNewExpression = ESString::create(u"NewExpression");
ESString*  astTypeProperty = ESString::create(u"Property");
ESString*  astTypeBinaryExpression = ESString::create(u"BinaryExpression");
ESString*  astTypeLogicalExpression = ESString::create(u"LogicalExpression");
ESString*  astTypeUpdateExpression = ESString::create(u"UpdateExpression");
ESString*  astTypeUnaryExpression = ESString::create(u"UnaryExpression");
ESString*  astTypeIfStatement = ESString::create(u"IfStatement");
ESString*  astTypeForStatement = ESString::create(u"ForStatement");
ESString*  astTypeForInStatement = ESString::create(u"ForInStatement");
ESString*  astTypeWhileStatement = ESString::create(u"WhileStatement");
ESString*  astTypeDoWhileStatement = ESString::create(u"DoWhileStatement");
ESString*  astTypeSwitchStatement = ESString::create(u"SwitchStatement");
ESString*  astTypeSwitchCase = ESString::create(u"SwitchCase");
ESString*  astTypeTryStatement = ESString::create(u"TryStatement");
ESString*  astTypeCatchClause = ESString::create(u"CatchClause");
ESString*  astTypeThrowStatement = ESString::create(u"ThrowStatement");
ESString*  astConditionalExpression = ESString::create(u"ConditionalExpression");

unsigned long getLongTickCount()
{
    struct timespec timespec;
    clock_gettime(CLOCK_MONOTONIC,&timespec);
    return (unsigned long)(timespec.tv_sec * 1000000L + timespec.tv_nsec/1000);
}

void ESScriptParser::enter()
{
}

void ESScriptParser::exit()
{
}

void ESScriptParser::gc()
{
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

std::string ESScriptParser::parseExternal(std::string& sourceString)
{
    RELEASE_ASSERT_NOT_REACHED();
}


Node* ESScriptParser::parseScript(ESVMInstance* instance, escargot::u16string& source)
{
    Node* node = esprima::parse(source);
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
                            //wprintf(L"outer function of this function  needs capture! -> because fn...%ls iden..%ls\n",
                            //        fn->id().data(),
                            //        ((IdentifierNode *)currentNode)->name().data());
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
        } else if(type == NodeType::BinaryExpression) {
            postAnalysisFunction(((BinaryExpressionNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
            postAnalysisFunction(((BinaryExpressionNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
        } else if(type == NodeType::LogicalExpression) {
            postAnalysisFunction(((LogicalExpressionNode *)currentNode)->m_right, identifierStack, nearFunctionNode);
            postAnalysisFunction(((LogicalExpressionNode *)currentNode)->m_left, identifierStack, nearFunctionNode);
        } else if(type == NodeType::UpdateExpression) {
            postAnalysisFunction(((UpdateExpressionNode *)currentNode)->m_argument, identifierStack, nearFunctionNode);
        } else if(type == NodeType::UnaryExpression) {
            postAnalysisFunction(((UnaryExpressionNode *)currentNode)->m_argument, identifierStack, nearFunctionNode);
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

    return node;
}

}
