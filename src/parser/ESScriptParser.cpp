#include "Escargot.h"
#include "ESScriptParser.h"

namespace escargot {

Node* ESScriptParser::parseScript(const std::string& source)
{
    std::string sc;
    for(unsigned i = 0 ; i < source.length() ; i ++) {
        char c = source[i];

        if(c == '\n') {
            sc.push_back('\\');
            c = '\n';
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

    fp = fopen("/tmp/input.js", "w");
    fputs(sourceString.c_str(), fp);
    fflush(fp);
    fclose(fp);

    char path[1035];

    fp = popen("./mozjs /tmp/input.js", "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    std::string outputString;
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
        outputString += path;
    }

    pclose(fp);

    ESString output = outputString.data();
    //output.show();

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
    ESString astTypeLiteral(L"Literal");
    ESString astTypeFunctionDeclaration(L"FunctionDeclaration");
    ESString astTypeFunctionExpression(L"FunctionExpression");
    ESString astTypeBlockStatement(L"BlockStatement");
    ESString astTypeArrayExpression(L"ArrayExpression");
    ESString astTypeCallExpression(L"CallExpression");
    ESString astTypeObjectExpression(L"ObjectExpression");
    ESString astTypeMemberExpression(L"MemberExpression");
    ESString astTypeProperty(L"Property");

    StatementNodeVector program_body;
    StatementNodeVector* current_body = &program_body;
    std::function<Node *(rapidjson::GenericValue<rapidjson::UTF16<>>& value)> fn;
    fn = [&](rapidjson::GenericValue<rapidjson::UTF16<>>& value) -> Node* {
        Node* parsedNode = NULL;
        ESString type(value[L"type"].GetString());
        if(type == astTypeProgram) {
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"body"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                Node* n = fn(children[i]);
                if (n != NULL) {
                	program_body.push_back(n);
                  }
            }
            parsedNode = new ProgramNode(std::move(program_body));
        } else if(type == astTypeVariableDeclaration) {
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"declarations"];
            VariableDeclaratorVector decl;
            ExpressionNodeVector assi;
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                decl.push_back(fn(children[i]));
                if (children[i][L"init"].GetType() != rapidjson::Type::kNullType) {
                    assi.push_back(new AssignmentExpressionNode(fn(children[i][L"id"]),
                            fn(children[i][L"init"]), AssignmentExpressionNode::AssignmentOperator::Equal));
                }
            }

            current_body->insert(current_body->begin(), new VariableDeclarationNode(std::move(decl)));

            if (assi.size() > 1) {
                parsedNode = new ExpressionStatementNode(new SequenceExpressionNode(std::move(assi)));
            } else if (assi.size() == 1) {
                parsedNode = new ExpressionStatementNode(assi[0]);
            } else {
                return NULL;
            }
        } else if(type == astTypeVariableDeclarator) {
            parsedNode = new VariableDeclaratorNode(fn(value[L"id"]));
        } else if(type == astTypeIdentifier) {
            parsedNode = new IdentifierNode(value[L"name"].GetString());
        } else if(type == astTypeExpressionStatement) {
            Node* node = fn(value[L"expression"]);
            parsedNode = new ExpressionStatementNode(node);
        } else if(type == astTypeAssignmentExpression) {
            parsedNode = new AssignmentExpressionNode(fn(value[L"left"]), fn(value[L"right"]), AssignmentExpressionNode::AssignmentOperator::Equal);
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
                parsedNode = new LiteralNode(String::create(value[L"value"].GetString()));
            } else if(value[L"value"].IsBool()) {
                parsedNode = new LiteralNode(Boolean::create(value[L"value"].GetBool()));
            } else if(value[L"value"].IsNull()) {
                parsedNode = new LiteralNode(null);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }

        } else if(type == astTypeFunctionDeclaration) {
            ESString id = value[L"id"][L"name"].GetString();
            ESStringVector params;

            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"params"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                params.push_back(children[i][L"name"].GetString());
              }

            Node* func_body = fn(value[L"body"]);
            current_body->insert(current_body->begin(), new FunctionDeclarationNode(id, std::move(params), func_body, value[L"generator"].GetBool(), value[L"generator"].GetBool()));
            return NULL;
        }  else if(type == astTypeFunctionExpression) {
            ESString id;
            ESStringVector params;

            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"params"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                params.push_back(children[i][L"name"].GetString());
              }

            Node* func_body = fn(value[L"body"]);
            parsedNode = new FunctionExpressionNode(id, std::move(params), func_body, value[L"generator"].GetBool(), value[L"generator"].GetBool());
        } else if(type == astTypeArrayExpression) {
            ExpressionNodeVector elems;
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"elements"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                elems.push_back(fn(children[i]));
            }
            parsedNode = new ArrayExpressionNode(std::move(elems));
        } else if(type == astTypeBlockStatement) {
            StatementNodeVector block_body;
            StatementNodeVector* outer_body = current_body;
            current_body = &block_body;
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"body"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                Node* n = fn(children[i]);
                if (n != NULL) {
                	block_body.push_back(n);
                	}
            	}
            current_body = outer_body;
            parsedNode = new BlockStatementNode(std::move(block_body));
        } else if(type == astTypeCallExpression) {
            Node* callee = fn(value[L"callee"]);
            ArgumentVector arguments;
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"arguments"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                arguments.push_back(fn(children[i]));
            }
            parsedNode = new CallExpressionNode(callee, std::move(arguments));
        } else if(type == astTypeObjectExpression) {
            PropertiesNodeVector propertiesVector;
            rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"properties"];
            for (rapidjson::SizeType i = 0; i < children.Size(); i++) {
                Node* n = fn(children[i]);
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
            parsedNode = new PropertyNode(fn(value[L"key"]), fn(value[L"value"]), kind);
        } else if(type == astTypeMemberExpression) {
            parsedNode = new MemberExpressionNode(fn(value[L"object"]), fn(value[L"property"]));
        }
#ifndef NDEBUG
        if(!parsedNode) {
            type.show();
        }
#endif
        RELEASE_ASSERT(parsedNode);
        return parsedNode;
    };

    return fn(jsonDocument);
}

}
