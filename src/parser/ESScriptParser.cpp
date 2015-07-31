#include "Escargot.h"
#include "ESScriptParser.h"
#include "vm/ESVMInstance.h"
#include "runtime/ESValue.h"

namespace escargot {

unsigned long getLongTickCount()
{
    struct timespec timespec;
    clock_gettime(CLOCK_MONOTONIC,&timespec);
    return (unsigned long)(timespec.tv_sec * 1000000L + timespec.tv_nsec/1000);
}


Node* ESScriptParser::parseScript(ESVMInstance* instance, const std::string& source)
{
    //unsigned long start = getLongTickCount();
    std::string sc;
    for(unsigned i = 0 ; i < source.length() ; i ++) {
        char c = source[i];

        if(c == '\n') {
            sc.push_back('\\');
            c = '\n';
        } else if(c == '\\') {
            sc.push_back('\\');
            c = '\\';
        } else if(c == '\"') {
            sc.push_back('\\');
            c = '\"';
        } else if(c == '\'') {
            sc.push_back('\\');
            c = '\'';
        } else if(c == '/') {
            if(i + 1 < source.length() && source[i + 1] == '/') {
                while(source[i] != '\n' && i < source.length()) {
                    i ++;
                }
                continue;
            }
            else if(i + 1 < source.length() && source[i + 1] == '*') {
                while(i < source.length()) {
                    if(source[i] == '*') {
                        if(i + 1 < source.length()) {
                            if(source[i + 1] == '/') {
                                i++;
                                break;
                            }
                        }
                    }
                    i ++;
                }
                continue;
            }

        }

        sc.push_back(c);
    }

    std::string sourceString = std::string("print(JSON.stringify(Reflect.parse('") + sc + "')))";

    FILE *fp;

    char fname[] = "/tmp/escargot_XXXXXX\0";
    const char* ptr = mkdtemp(fname);
    char prefix[4096];
    strcpy(prefix,ptr);
    strcat(prefix,"/input.js");

    fp = fopen(prefix, "w");
    fputs(sourceString.c_str(), fp);
    fflush(fp);
    fclose(fp);

    char path[1035];
    char filePath[1035];
    strcpy(filePath,"./mozjs ");
    strcat(filePath, prefix);

    fp = popen(filePath, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    std::string outputString;
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
        outputString += path;
    }

    pclose(fp);

    remove(prefix);
    rmdir(ptr);

    InternalString output = outputString.data();
    //output.show();

    rapidjson::GenericDocument<rapidjson::UTF16<>> jsonDocument;
    rapidjson::GenericStringStream<rapidjson::UTF16<>> stringStream(output.data());
    jsonDocument.ParseStream(stringStream);

    //READ SAMPLE
    //std::wstring type = jsonDocument[L"type"].GetString();
    //wprintf(L"%ls\n",type.data());

    //TODO move these strings into elsewhere
    InternalString astTypeProgram(L"Program");
    InternalString astTypeVariableDeclaration(L"VariableDeclaration");
    InternalString astTypeExpressionStatement(L"ExpressionStatement");
    InternalString astTypeVariableDeclarator(L"VariableDeclarator");
    InternalString astTypeIdentifier(L"Identifier");
    InternalString astTypeAssignmentExpression(L"AssignmentExpression");
    InternalString astTypeThisExpression(L"ThisExpression");
    InternalString astTypeReturnStatement(L"ReturnStatement");
    InternalString astTypeEmptyStatement(L"EmptyStatement");
    InternalString astTypeLiteral(L"Literal");
    InternalString astTypeFunctionDeclaration(L"FunctionDeclaration");
    InternalString astTypeFunctionExpression(L"FunctionExpression");
    InternalString astTypeBlockStatement(L"BlockStatement");
    InternalString astTypeArrayExpression(L"ArrayExpression");
    InternalString astTypeCallExpression(L"CallExpression");
    InternalString astTypeObjectExpression(L"ObjectExpression");
    InternalString astTypeMemberExpression(L"MemberExpression");
    InternalString astTypeNewExpression(L"NewExpression");
    InternalString astTypeProperty(L"Property");
    InternalString astTypeBinaryExpression(L"BinaryExpression");
    InternalString astTypeUpdateExpression(L"UpdateExpression");
    InternalString astTypeIfStatement(L"IfStatement");
    InternalString astTypeForStatement(L"ForStatement");
    InternalString astTypeWhileStatement(L"WhileStatement");
    InternalString astTypeTryStatement(L"TryStatement");
    InternalString astTypeCatchClause(L"CatchClause");
    InternalString astTypeThrowStatement(L"ThrowStatement");

    StatementNodeVector programBody;
    std::function<Node *(rapidjson::GenericValue<rapidjson::UTF16<>>& value, StatementNodeVector* currentBody, bool shouldGenerateNewBody)> fn;
    fn = [&](rapidjson::GenericValue<rapidjson::UTF16<>>& value, StatementNodeVector* currentBody, bool shouldGenerateNewBody) -> Node* {
        Node* parsedNode = NULL;
        InternalString type(value[L"type"].GetString());
        if(type == astTypeProgram) {
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"body"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                Node* n = fn(children[i], currentBody, false);
                if (n != NULL) {
                    programBody.push_back(n);
                  }
            }
            parsedNode = new ProgramNode(std::move(programBody));
        } else if(type == astTypeVariableDeclaration) {
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"declarations"];
            VariableDeclaratorVector decl;
            ExpressionNodeVector assi;
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                decl.push_back(fn(children[i], currentBody, false));
                if (children[i][L"init"].GetType() != rapidjson::Type::kNullType) {
                    assi.push_back(new AssignmentExpressionNode(fn(children[i][L"id"], currentBody, false),
                            fn(children[i][L"init"], currentBody, false), L"="));
                }
            }

            currentBody->insert(currentBody->begin(), new VariableDeclarationNode(std::move(decl)));

            if (assi.size() > 1) {
                parsedNode = new ExpressionStatementNode(new SequenceExpressionNode(std::move(assi)));
            } else if (assi.size() == 1) {
                parsedNode = new ExpressionStatementNode(assi[0]);
            } else {
                return NULL;
            }
        } else if(type == astTypeVariableDeclarator) {
            parsedNode = new VariableDeclaratorNode(fn(value[L"id"], currentBody, false));
        } else if(type == astTypeIdentifier) {
            parsedNode = new IdentifierNode(std::wstring(value[L"name"].GetString()));
        } else if(type == astTypeExpressionStatement) {
            Node* node = fn(value[L"expression"], currentBody, false);
            parsedNode = new ExpressionStatementNode(node);
        } else if(type == astTypeAssignmentExpression) {
            parsedNode = new AssignmentExpressionNode(fn(value[L"left"], currentBody, false), fn(value[L"right"], currentBody, false), value[L"operator"].GetString());
        } else if(type == astTypeLiteral) {
            //TODO parse esvalue better
            if(value[L"value"].IsNumber()) {
                double number = value[L"value"].GetDouble();
                if(std::abs(number) < 0xC0000000 && value[L"value"].IsInt()) { //(1100)(0000)...(0000)
                    parsedNode = new LiteralNode(Smi::fromInt(value[L"value"].GetInt()));
                } else {
                    parsedNode = new LiteralNode(ESNumber::create(number));
                }
            } else if(value[L"value"].IsString()) {
                parsedNode = new LiteralNode(ESString::create(value[L"value"].GetString()));
            } else if(value[L"value"].IsBool()) {
                parsedNode = new LiteralNode(ESBoolean::create(value[L"value"].GetBool()));
            } else if(value[L"value"].IsNull()) {
                parsedNode = new LiteralNode(esESNull);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }

        } else if(type == astTypeFunctionDeclaration) {
            InternalAtomicString id = InternalAtomicString(value[L"id"][L"name"].GetString());
            InternalAtomicStringVector params;

            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"params"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                params.push_back(std::wstring(children[i][L"name"].GetString()));
            }

            Node* func_body = fn(value[L"body"], currentBody, true);
            currentBody->insert(currentBody->begin(), new FunctionDeclarationNode(id, std::move(params), func_body, value[L"generator"].GetBool(), value[L"generator"].GetBool()));
            return NULL;
        }  else if(type == astTypeFunctionExpression) {
            InternalAtomicString id;
            InternalAtomicStringVector params;

            if(!value[L"id"].IsNull())
                id = InternalAtomicString(value[L"id"][L"name"].GetString());

            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"params"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                params.push_back(InternalAtomicString(children[i][L"name"].GetString()));
            }

            Node* func_body = fn(value[L"body"], currentBody, true);
            parsedNode = new FunctionExpressionNode(id, std::move(params), func_body, value[L"generator"].GetBool(), value[L"generator"].GetBool());
        } else if(type == astTypeArrayExpression) {
            ExpressionNodeVector elems;
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"elements"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                elems.push_back(fn(children[i], currentBody, false));
            }
            parsedNode = new ArrayExpressionNode(std::move(elems));
        } else if(type == astTypeBlockStatement) {
            StatementNodeVector blockBody;
            StatementNodeVector* old = currentBody;

            if(shouldGenerateNewBody)
                currentBody = &blockBody;
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"body"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                Node* n = fn(children[i], currentBody, false);
                if (n != NULL) {
                    blockBody.push_back(n);
                }
            }
            if(shouldGenerateNewBody)
                currentBody = old;
            parsedNode = new BlockStatementNode(std::move(blockBody));
        } else if(type == astTypeCallExpression) {
            Node* callee = fn(value[L"callee"], currentBody, false);
            ArgumentVector arguments;
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"arguments"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                arguments.push_back(fn(children[i], currentBody, false));
            }
            parsedNode = new CallExpressionNode(callee, std::move(arguments));
        } else if(type == astTypeNewExpression) {
            Node* callee = fn(value[L"callee"], currentBody, false);
            ArgumentVector arguments;
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"arguments"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                arguments.push_back(fn(children[i], currentBody, false));
            }
            parsedNode = new NewExpressionNode(callee, std::move(arguments));
        } else if(type == astTypeObjectExpression) {
            PropertiesNodeVector propertiesVector;
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"properties"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                Node* n = fn(children[i], currentBody, false);
                ASSERT(n->type() == NodeType::Property);
                propertiesVector.push_back((PropertyNode *)n);
            }
            parsedNode = new ObjectExpressionNode(std::move(propertiesVector));
        } else if(type == astTypeProperty) {
            PropertyNode::Kind kind = PropertyNode::Kind::Init;
            if(std::wstring(L"get") == value[L"kind"].GetString()) {
                kind = PropertyNode::Kind::Get;
            } else if(std::wstring(L"set") == value[L"kind"].GetString()) {
                kind = PropertyNode::Kind::Set;
            }
            parsedNode = new PropertyNode(fn(value[L"key"], currentBody, false), fn(value[L"value"], currentBody, false), kind);
        } else if(type == astTypeMemberExpression) {
            parsedNode = new MemberExpressionNode(fn(value[L"object"], currentBody, false), fn(value[L"property"], currentBody, false), value[L"computed"].GetBool());
        } else if(type == astTypeBinaryExpression) {
            parsedNode = new BinaryExpressionNode(fn(value[L"left"], currentBody, false), fn(value[L"right"], currentBody, false), value[L"operator"].GetString());
        } else if(type == astTypeUpdateExpression) {
            parsedNode = new UpdateExpressionNode(fn(value[L"argument"], currentBody, false), value[L"operator"].GetString(), value[L"prefix"].GetBool());
        } else if(type == astTypeIfStatement) {
            parsedNode = new IfStatementNode(fn(value[L"test"], currentBody, false), fn(value[L"consequent"], currentBody, false), value[L"alternate"].IsNull()? NULL : fn(value[L"alternate"], currentBody, false));
        } else if(type == astTypeForStatement) {
            parsedNode = new ForStatementNode(fn(value[L"init"], currentBody, false), fn(value[L"test"], currentBody, false), fn(value[L"update"], currentBody, false), fn(value[L"body"], currentBody, false));
        } else if(type == astTypeWhileStatement) {
            parsedNode = new WhileStatementNode(fn(value[L"test"], currentBody, false), fn(value[L"body"], currentBody, false));
        } else if(type == astTypeThisExpression) {
            parsedNode = new ThisExpressionNode();
        } else if(type == astTypeReturnStatement) {
            parsedNode = new ReturnStatmentNode(fn(value[L"argument"], currentBody, false));
        } else if(type == astTypeEmptyStatement) {
            parsedNode = new EmptyStatementNode();
        } else if (type == astTypeTryStatement) {
           CatchClauseNodeVector guardedHandlers;
           rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"guardedHandlers"];
           for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                guardedHandlers.push_back(fn(children[i], currentBody, false));
           }
           if (value[L"finalizer"].IsNull()) {
               parsedNode = new TryStatementNode(fn(value[L"block"], currentBody, false), fn(value[L"handler"], currentBody, false), std::move(guardedHandlers), NULL);
           } else {
               parsedNode = new TryStatementNode(fn(value[L"block"], currentBody, false), fn(value[L"handler"], currentBody, false), std::move(guardedHandlers), fn(value[L"finalizer"], currentBody, false));
           }
        } else if (type == astTypeCatchClause) {
            if (value[L"guard"].IsNull()) {
                parsedNode = new CatchClauseNode(fn(value[L"param"], currentBody, false), NULL, fn(value[L"body"], currentBody, false));
            } else {
                parsedNode = new CatchClauseNode(fn(value[L"param"], currentBody, false), fn(value[L"guard"], currentBody, false), fn(value[L"body"], currentBody, false));
            }
        } else if (type == astTypeThrowStatement) {
            parsedNode = new ThrowStatementNode(fn(value[L"argument"], currentBody, false));
        }
#ifndef NDEBUG
        if(!parsedNode) {
            type.show();
        }
#endif
        RELEASE_ASSERT(parsedNode);
        return parsedNode;
    };

    //parse
    Node* node = fn(jsonDocument, &programBody, false);

    auto markNeedsActivation = [](FunctionNode* nearFunctionNode){
        FunctionNode* node = nearFunctionNode;
        while(node) {
            node->setNeedsActivation(true);
            node = node->outerFunctionNode();
        }
    };

    std::function<void (Node* currentNode, InternalAtomicStringVector& identifierInCurrentContext, FunctionNode* nearFunctionNode)> postAnalysisFunction =
            [&postAnalysisFunction, instance, &markNeedsActivation](Node* currentNode, InternalAtomicStringVector& identifierInCurrentContext, FunctionNode* nearFunctionNode) {
        if(!currentNode)
            return;
        NodeType type = currentNode->type();
        if(type == NodeType::Program) {
            StatementNodeVector& v = ((ProgramNode *)currentNode)->m_body;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierInCurrentContext, nearFunctionNode);
            }
        } else if(type == NodeType::VariableDeclaration) {
            VariableDeclaratorVector& v = ((VariableDeclarationNode *)currentNode)->m_declarations;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierInCurrentContext, nearFunctionNode);
            }
        } else if(type == NodeType::VariableDeclarator) {
            //
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
            postAnalysisFunction(((FunctionDeclarationNode *)currentNode)->m_body, newIdentifierVector, ((FunctionDeclarationNode *)currentNode));
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
            postAnalysisFunction(((FunctionExpressionNode *)currentNode)->m_body, newIdentifierVector, ((FunctionExpressionNode *)currentNode));
            ((FunctionExpressionNode *)currentNode)->setInnerIdentifiers(std::move(newIdentifierVector));
            //wprintf(L"end of process function body-------------------\n");
        } else if(type == NodeType::Identifier) {
            //use case
            InternalAtomicString name = ((IdentifierNode *)currentNode)->name();
            auto iter = std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),name);
            if(name == strings->arguments && iter == identifierInCurrentContext.end() && nearFunctionNode) {
                identifierInCurrentContext.push_back(strings->arguments);
                nearFunctionNode->markNeedsArgumentsObject();
                iter = std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),name);
            }
            if(identifierInCurrentContext.end() == iter) {
                if(!instance->globalObject()->hasKey(name)) {
                    if(nearFunctionNode && nearFunctionNode->outerFunctionNode()) {
                        //wprintf(L"this function  needs capture! -> %ls\n", ((IdentifierNode *)currentNode)->name().data());
                        markNeedsActivation(nearFunctionNode->outerFunctionNode());
                    }
                }
            } else {
                if(nearFunctionNode) {
                    size_t idx = std::distance(identifierInCurrentContext.begin(), iter);
                    ((IdentifierNode *)currentNode)->setFastAccessIndex(idx);
                }
            }
            //wprintf(L"use Identifier %ls\n", ((IdentifierNode *)currentNode)->name().data());
        } else if(type == NodeType::ExpressionStatement) {
            postAnalysisFunction(((ExpressionStatementNode *)currentNode)->m_expression, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::AssignmentExpression) {
            postAnalysisFunction(((AssignmentExpressionNode *)currentNode)->m_right, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((AssignmentExpressionNode *)currentNode)->m_left, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::Literal) {
            //DO NOTHING
        }else if(type == NodeType::ArrayExpression) {
            ExpressionNodeVector& v = ((ArrayExpressionNode *)currentNode)->m_elements;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierInCurrentContext, nearFunctionNode);
            }
        } else if(type == NodeType::BlockStatement) {
            StatementNodeVector& v = ((BlockStatementNode *)currentNode)->m_body;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierInCurrentContext, nearFunctionNode);
            }
        } else if(type == NodeType::CallExpression) {

            Node * callee = ((CallExpressionNode *)currentNode)->m_callee;
            if(callee) {
                if(callee->type() == NodeType::Identifier) {
                    if(((IdentifierNode *)callee)->name() == InternalAtomicString(L"eval")) {
                        markNeedsActivation(nearFunctionNode);
                    }
                }
            }

            postAnalysisFunction(callee, identifierInCurrentContext, nearFunctionNode);
            ArgumentVector& v = ((CallExpressionNode *)currentNode)->m_arguments;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierInCurrentContext, nearFunctionNode);
            }
        } else if(type == NodeType::SequenceExpression) {
            ExpressionNodeVector& v = ((SequenceExpressionNode *)currentNode)->m_expressions;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierInCurrentContext, nearFunctionNode);
            }
        } else if(type == NodeType::NewExpression) {
            postAnalysisFunction(((NewExpressionNode *)currentNode)->m_callee, identifierInCurrentContext, nearFunctionNode);
            ArgumentVector& v = ((NewExpressionNode *)currentNode)->m_arguments;
            for(unsigned i = 0; i < v.size() ; i ++) {
                postAnalysisFunction(v[i], identifierInCurrentContext, nearFunctionNode);
            }
        } else if(type == NodeType::ObjectExpression) {
            PropertiesNodeVector& v = ((ObjectExpressionNode *)currentNode)->m_properties;
            for(unsigned i = 0; i < v.size() ; i ++) {
                PropertyNode* p = v[i];
                postAnalysisFunction(p->value(), identifierInCurrentContext, nearFunctionNode);
                if(p->key()->type() == NodeType::Identifier) {

                } else {
                    postAnalysisFunction(p->key(), identifierInCurrentContext, nearFunctionNode);
                }
            }
        } else if(type == NodeType::Property) {
            postAnalysisFunction(((PropertyNode *)currentNode)->m_key, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((PropertyNode *)currentNode)->m_value, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::MemberExpression) {
            postAnalysisFunction(((MemberExpressionNode *)currentNode)->m_object, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((MemberExpressionNode *)currentNode)->m_property, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::BinaryExpression) {
            postAnalysisFunction(((BinaryExpressionNode *)currentNode)->m_right, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((BinaryExpressionNode *)currentNode)->m_left, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::UpdateExpression) {
            postAnalysisFunction(((UpdateExpressionNode *)currentNode)->m_argument, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::IfStatement) {
            postAnalysisFunction(((IfStatementNode *)currentNode)->m_test, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((IfStatementNode *)currentNode)->m_consequente, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((IfStatementNode *)currentNode)->m_alternate, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::ForStatement) {
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_init, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_body, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_test, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_update, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::WhileStatement) {
            postAnalysisFunction(((WhileStatementNode *)currentNode)->m_test, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((WhileStatementNode *)currentNode)->m_body, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::ThisExpression) {

        } else if(type == NodeType::ReturnStatement) {
            postAnalysisFunction(((ReturnStatmentNode *)currentNode)->m_argument, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::EmptyStatement) {
        } else if (type == NodeType::TryStatement) {
            postAnalysisFunction(((TryStatementNode *)currentNode)->m_block, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((TryStatementNode *)currentNode)->m_handler, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((TryStatementNode *)currentNode)->m_finalizer, identifierInCurrentContext, nearFunctionNode);
        } else if (type == NodeType::CatchClause) {
            markNeedsActivation(nearFunctionNode);
            postAnalysisFunction(((CatchClauseNode *)currentNode)->m_param, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((CatchClauseNode *)currentNode)->m_guard, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((CatchClauseNode *)currentNode)->m_body, identifierInCurrentContext, nearFunctionNode);
        } else if (type == NodeType::ThrowStatement) {
            postAnalysisFunction(((ThrowStatementNode *)currentNode)->m_argument, identifierInCurrentContext, nearFunctionNode);
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    };

    InternalAtomicStringVector identifierInCurrentContext;
    postAnalysisFunction(node, identifierInCurrentContext, NULL);

    //unsigned long end = getLongTickCount();
    //fwprintf(stderr, L"parse script takes %g ms\n", (end - start)/1000.f);
    return node;
}

}
