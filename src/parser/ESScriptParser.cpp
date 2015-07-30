#include "Escargot.h"
#include "ESScriptParser.h"

namespace escargot {

unsigned long getLongTickCount()
{
    struct timespec timespec;
    clock_gettime(CLOCK_MONOTONIC,&timespec);
    return (unsigned long)(timespec.tv_sec * 1000000L + timespec.tv_nsec/1000);
}


Node* ESScriptParser::parseScript(const std::string& source)
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

    ESString output = outputString.data();
    //output.show();

    //unsigned long end = getLongTickCount();

    //fwprintf(stderr, L"calling mozjs takes %g ms\n", (end - start)/1000.f);

    rapidjson::GenericDocument<rapidjson::UTF16<>> jsonDocument;
    rapidjson::GenericStringStream<rapidjson::UTF16<>> stringStream(output.data());
    jsonDocument.ParseStream(stringStream);

    //READ SAMPLE
    //std::wstring type = jsonDocument[L"type"].GetString();
    //wprintf(L"%ls\n",type.data());

    //TODO move these strings into elsewhere
    ESString astTypeProgram(L"Program");
    ESString astTypeVariableDeclaration(L"VariableDeclaration");
    ESString astTypeExpressionStatement(L"ExpressionStatement");
    ESString astTypeVariableDeclarator(L"VariableDeclarator");
    ESString astTypeIdentifier(L"Identifier");
    ESString astTypeAssignmentExpression(L"AssignmentExpression");
    ESString astTypeThisExpression(L"ThisExpression");
    ESString astTypeReturnStatement(L"ReturnStatement");
    ESString astTypeEmptyStatement(L"EmptyStatement");
    ESString astTypeLiteral(L"Literal");
    ESString astTypeFunctionDeclaration(L"FunctionDeclaration");
    ESString astTypeFunctionExpression(L"FunctionExpression");
    ESString astTypeBlockStatement(L"BlockStatement");
    ESString astTypeArrayExpression(L"ArrayExpression");
    ESString astTypeCallExpression(L"CallExpression");
    ESString astTypeObjectExpression(L"ObjectExpression");
    ESString astTypeMemberExpression(L"MemberExpression");
    ESString astTypeNewExpression(L"NewExpression");
    ESString astTypeProperty(L"Property");
    ESString astTypeBinaryExpression(L"BinaryExpression");
    ESString astTypeUpdateExpression(L"UpdateExpression");
    ESString astTypeIfStatement(L"IfStatement");
    ESString astTypeForStatement(L"ForStatement");
    ESString astTypeWhileStatement(L"WhileStatement");
    ESString astTypeTryStatement(L"TryStatement");
    ESString astTypeCatchClause(L"CatchClause");
    ESString astTypeThrowStatement(L"ThrowStatement");

    StatementNodeVector programBody;
    std::function<Node *(rapidjson::GenericValue<rapidjson::UTF16<>>& value, StatementNodeVector* currentBody, bool shouldGenerateNewBody)> fn;
    fn = [&](rapidjson::GenericValue<rapidjson::UTF16<>>& value, StatementNodeVector* currentBody, bool shouldGenerateNewBody) -> Node* {
        Node* parsedNode = NULL;
        ESString type(value[L"type"].GetString());
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
                    parsedNode = new LiteralNode(Number::create(number));
                }
            } else if(value[L"value"].IsString()) {
                parsedNode = new LiteralNode(PString::create(value[L"value"].GetString()));
            } else if(value[L"value"].IsBool()) {
                parsedNode = new LiteralNode(PBoolean::create(value[L"value"].GetBool()));
            } else if(value[L"value"].IsNull()) {
                parsedNode = new LiteralNode(esNull);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }

        } else if(type == astTypeFunctionDeclaration) {
            ESAtomicString id = ESAtomicString(value[L"id"][L"name"].GetString());
            ESAtomicStringVector params;

            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"params"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                params.push_back(std::wstring(children[i][L"name"].GetString()));
            }

            Node* func_body = fn(value[L"body"], currentBody, true);
            currentBody->insert(currentBody->begin(), new FunctionDeclarationNode(id, std::move(params), func_body, value[L"generator"].GetBool(), value[L"generator"].GetBool()));
            return NULL;
        }  else if(type == astTypeFunctionExpression) {
            ESAtomicString id;
            ESAtomicStringVector params;

            if(!value[L"id"].IsNull())
                id = ESAtomicString(value[L"id"][L"name"].GetString());

            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"params"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                params.push_back(ESAtomicString(children[i][L"name"].GetString()));
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

    return fn(jsonDocument, &programBody, false);
}

}
