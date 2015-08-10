#include "Escargot.h"
#include "ESScriptParser.h"
#include "vm/ESVMInstance.h"
#include "runtime/ESValue.h"

#include "jsapi.h"

namespace escargot {

::JSContext* ESScriptParser::s_cx;
::JSRuntime* ESScriptParser::s_rt;
::JSObject* ESScriptParser::s_reflectObject;
::JSFunction* ESScriptParser::s_reflectParseFunction;

InternalString astTypeProgram(L"Program");
InternalString astTypeVariableDeclaration(L"VariableDeclaration");
InternalString astTypeExpressionStatement(L"ExpressionStatement");
InternalString astTypeVariableDeclarator(L"VariableDeclarator");
InternalString astTypeIdentifier(L"Identifier");
InternalString astTypeAssignmentExpression(L"AssignmentExpression");
InternalString astTypeThisExpression(L"ThisExpression");
InternalString astTypeBreakStatement(L"BreakStatement");
InternalString astTypeContinueStatement(L"ContinueStatement");
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
InternalString astTypeLogicalExpression(L"LogicalExpression");
InternalString astTypeUpdateExpression(L"UpdateExpression");
InternalString astTypeUnaryExpression(L"UnaryExpression");
InternalString astTypeIfStatement(L"IfStatement");
InternalString astTypeForStatement(L"ForStatement");
InternalString astTypeForInStatement(L"ForInStatement");
InternalString astTypeWhileStatement(L"WhileStatement");
InternalString astTypeDoWhileStatement(L"DoWhileStatement");
InternalString astTypeTryStatement(L"TryStatement");
InternalString astTypeCatchClause(L"CatchClause");
InternalString astTypeThrowStatement(L"ThrowStatement");
InternalString astConditionalExpression(L"ConditionalExpression");

void* ESScriptParser::s_global;

unsigned long getLongTickCount()
{
    struct timespec timespec;
    clock_gettime(CLOCK_MONOTONIC,&timespec);
    return (unsigned long)(timespec.tv_sec * 1000000L + timespec.tv_nsec/1000);
}

static JSClass global_class = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    // [SpiderMonkey 38] Following Stubs are removed. Remove those lines.
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

void ESScriptParser::enter()
{
    const unsigned mem = 8L * 1024 * 1024;
    s_rt = JS_NewRuntime(mem, JS_NO_HELPER_THREADS);
    if (!s_rt)
        ::exit(0);

    s_cx = JS_NewContext(s_rt, mem);
    if (!s_cx)
        ::exit(0);

    s_global = new JS::RootedObject(s_cx, JS_NewGlobalObject(s_cx, &global_class, nullptr));
    if (!s_global)
        ::exit(0);

    {
        JSAutoCompartment ac(s_cx, *((JS::RootedObject*)s_global));
        JS_InitStandardClasses(s_cx, *((JS::RootedObject*)s_global));
        JS_InitReflect(s_cx, *((JS::RootedObject*)s_global));
        jsval r;
        JS_GetProperty(s_cx, *((JS::RootedObject*)s_global), "Reflect", &r);
        s_reflectObject = JSVAL_TO_OBJECT(r);
        JS_GetProperty(s_cx, s_reflectObject, "parse", &r);
        s_reflectParseFunction = JS_ValueToFunction(s_cx, r);
    }
}

void ESScriptParser::exit()
{
    JS_DestroyContext(s_cx);
    JS_DestroyRuntime(s_rt);
    JS_ShutDown();
}

std::string ESScriptParser::parseExternal(std::string& sourceString)
{
    JS::RootedValue rval(s_cx);
    {
        JSAutoCompartment ac(s_cx, *((JS::RootedObject*)s_global));
        const char *script = sourceString.c_str();
        const char *filename = "noname";
        int lineno = 1;
        bool ok = JS_EvaluateScript(s_cx, *((JS::RootedObject*)s_global), script, strlen(script), filename, lineno, rval.address());
        if (!ok)
            return "!ok";
    }
    JSString *str = rval.toString();
    return JS_EncodeString(s_cx, str);
}


ALWAYS_INLINE const char* getStringFromMozJS(JSContext* ctx, JSObject* obj, const char* name)
{
    jsval val;
    JS_GetProperty(ctx, obj, name, &val);
    JSString* jsType = JSVAL_TO_STRING(val);
    return JS_EncodeString(ctx, jsType);
}

ALWAYS_INLINE bool getBooleanFromMozJS(JSContext* ctx, JSObject* obj, const char* name)
{
    jsval val;
    JS_GetProperty(ctx, obj, name, &val);
    return JSVAL_TO_BOOLEAN(val);
}

ALWAYS_INLINE JSObject* getObjectFromMozJS(JSContext* ctx, JSObject* obj, const char* name)
{
    jsval val;
    JS_GetProperty(ctx, obj, name, &val);
    JSObject* o;
    JS_ValueToObject(ctx,val,&o);
    return o;
}

ALWAYS_INLINE uint32_t getArrayLengthFromMozJS(JSContext* ctx, JSObject* obj)
{
    jsval val;
    uint32_t len;
    JS_GetArrayLength(ctx, obj, &len);
    return len;
}

ALWAYS_INLINE JSObject* getArrayElementFromMozJS(JSContext* ctx, JSObject* obj, uint32_t idx)
{
    jsval v;
    JS_GetElement(ctx, obj, idx, &v);
    JSObject* o;
    JS_ValueToObject(ctx,v,&o);
    return o;
}

ALWAYS_INLINE bool hasElementInMozJS(JSContext* ctx, JSObject* obj, const char* name)
{
    JSBool result;
    JS_HasProperty(ctx, obj, name, &result);
    return result;
}

Node* ESScriptParser::parseScript(ESVMInstance* instance, const std::string& source)
{
    //unsigned long start = getLongTickCount();

    JSAutoCompartment ac(s_cx, *((JS::RootedObject*)s_global));
    jsval ret;
    JSString * jsSrcStr = JS_InternString(s_cx, source.c_str());
    jsval srcStr = STRING_TO_JSVAL(jsSrcStr);
    jsval argv[1] = {srcStr};
    JS_CallFunction(s_cx, *((JS::RootedObject*)s_global), s_reflectParseFunction, 1, argv ,&ret);

    StatementNodeVector programBody;
    std::function<Node *(::JSObject *, StatementNodeVector* currentBody, bool shouldGenerateNewBody)> fn;
    fn = [&](::JSObject * obj, StatementNodeVector* currentBody, bool shouldGenerateNewBody) -> Node* {
        Node* parsedNode = NULL;
        InternalString type(getStringFromMozJS(s_cx, obj, "type"));

        if(type == astTypeProgram) {
            JSObject* childObj = getObjectFromMozJS(s_cx, obj, "body");
            uint32_t siz = getArrayLengthFromMozJS(s_cx, childObj);
            for (uint32_t i = 0; i < siz; i++) {
                Node* n = fn(getArrayElementFromMozJS(s_cx, childObj, i), currentBody, false);
                if (n != NULL) {
                    programBody.push_back(n);
                }
            }
            parsedNode = new ProgramNode(std::move(programBody));
        } else if(type == astTypeVariableDeclaration) {
            //rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"declarations"];
            JSObject* children = getObjectFromMozJS(s_cx, obj, "declarations");
            VariableDeclaratorVector decl;
            ExpressionNodeVector assi;
            uint32_t siz = getArrayLengthFromMozJS(s_cx, children);
            for (uint32_t i = 0; i < siz; i++) {
                JSObject* c = getArrayElementFromMozJS(s_cx, children, i);
                decl.push_back(fn(c, currentBody, false));
                jsval v;
                JS_GetProperty(s_cx, obj, "init", &v);
                if (getObjectFromMozJS(s_cx, c, "init")) {
                    assi.push_back(new AssignmentExpressionNode(fn(getObjectFromMozJS(s_cx, c, "id"), currentBody, false),
                            fn(getObjectFromMozJS(s_cx, c, "init"), currentBody, false), L"="));
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
            parsedNode = new VariableDeclaratorNode(fn(getObjectFromMozJS(s_cx, obj, "id"), currentBody, false));
        } else if(type == astTypeIdentifier) {
            InternalString is(getStringFromMozJS(s_cx, obj, "name"));
            parsedNode = new IdentifierNode(std::wstring(is.data()));
        } else if(type == astTypeExpressionStatement) {
            Node* node = fn(getObjectFromMozJS(s_cx, obj, "expression"), currentBody, false);
            parsedNode = new ExpressionStatementNode(node);
        } else if(type == astTypeAssignmentExpression) {
            parsedNode = new AssignmentExpressionNode(fn(getObjectFromMozJS(s_cx, obj, "left"), currentBody, false),
                    fn(getObjectFromMozJS(s_cx, obj, "right"), currentBody, false), InternalString(getStringFromMozJS(s_cx, obj, "operator")));
        } else if(type == astTypeLiteral) {
            //TODO parse esvalue better
            jsval v;
            JS_GetProperty(s_cx, obj, "value", &v);


            if(JSVAL_IS_INT(v)) {
                int number = JSVAL_TO_INT(v);
                parsedNode = new LiteralNode(ESValue(number));
            } else if(JSVAL_IS_DOUBLE(v)) {
                double number = JSVAL_TO_DOUBLE(v);
                parsedNode = new LiteralNode(ESValue(number));
            } else if(JSVAL_IS_STRING(v)) {
                JSString* ss = JSVAL_TO_STRING(v);
                InternalString is(JS_EncodeString(s_cx, ss));
                parsedNode = new LiteralNode(ESValue(ESString::create(is)));
            } else if(JSVAL_IS_BOOLEAN(v)) {
                JSBool b = JSVAL_TO_BOOLEAN(v);
                if(b)
                    parsedNode = new LiteralNode(ESValue(ESValue::ESTrueTag::ESTrue));
                else
                    parsedNode = new LiteralNode(ESValue(ESValue::ESFalseTag::ESFalse));
            } else if(JSVAL_IS_NULL(v)) {
                parsedNode = new LiteralNode(ESValue(ESValue::ESNullTag::ESNull));
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }

        } else if(type == astTypeFunctionDeclaration) {
            JSObject* idObj = getObjectFromMozJS(s_cx, obj, "id");
            InternalString is(getStringFromMozJS(s_cx, idObj, "name"));
            InternalAtomicString id = InternalAtomicString(is.data());
            InternalAtomicStringVector params;

            //rapidjson::GenericValue<rapidjson::UTF16<>>& children = value[L"params"];
            JSObject* children = getObjectFromMozJS(s_cx, obj, "params");
            uint32_t siz = getArrayLengthFromMozJS(s_cx, children);
            for (uint32_t i = 0; i < siz; i++) {
                InternalString is(getStringFromMozJS(s_cx, getArrayElementFromMozJS(s_cx, children, i), "name"));
                params.push_back(is.data());
            }

            Node* func_body = fn(getObjectFromMozJS(s_cx, obj, "body"), currentBody, true);
            currentBody->insert(currentBody->begin(), new FunctionDeclarationNode(id, std::move(params), func_body, getBooleanFromMozJS(s_cx, obj, "generator")
                    , getBooleanFromMozJS(s_cx, obj, "expression"), false));
            return NULL;
        }  else if(type == astTypeFunctionExpression) {
            InternalAtomicString id;
            InternalAtomicStringVector params;

            if(getObjectFromMozJS(s_cx, obj, "id"))
                id = InternalAtomicString(InternalString(getStringFromMozJS(s_cx, getObjectFromMozJS(s_cx, obj, "id"), "name")).data());

            JSObject* children = getObjectFromMozJS(s_cx, obj, "params");
            uint32_t siz = getArrayLengthFromMozJS(s_cx, children);
            for (uint32_t i = 0; i < siz; i++) {
                InternalString is(getStringFromMozJS(s_cx, getArrayElementFromMozJS(s_cx, children, i), "name"));
                params.push_back(is.data());
            }

            Node* func_body = fn(getObjectFromMozJS(s_cx, obj, "body"), currentBody, true);
            parsedNode = new FunctionExpressionNode(id, std::move(params), func_body, getBooleanFromMozJS(s_cx, obj, "generator")
                    , getBooleanFromMozJS(s_cx, obj, "expression"));
        } else if(type == astTypeArrayExpression) {
            ExpressionNodeVector elems;
            JSObject* children = getObjectFromMozJS(s_cx, obj, "elements");
            uint32_t siz = getArrayLengthFromMozJS(s_cx, children);
            for (uint32_t i = 0; i < siz; i++) {
                elems.push_back(fn(getArrayElementFromMozJS(s_cx, children, i), currentBody, false));
            }
            parsedNode = new ArrayExpressionNode(std::move(elems));
        } else if(type == astTypeBlockStatement) {
            StatementNodeVector blockBody;
            StatementNodeVector* old = currentBody;

            if(shouldGenerateNewBody)
                currentBody = &blockBody;
            JSObject* children = getObjectFromMozJS(s_cx, obj, "body");
            uint32_t siz = getArrayLengthFromMozJS(s_cx, children);
            for (uint32_t i = 0; i < siz; i++) {
                Node* n = fn(getArrayElementFromMozJS(s_cx, children, i), currentBody, false);
                if (n != NULL) {
                    blockBody.push_back(n);
                }
            }
            if(shouldGenerateNewBody)
                currentBody = old;
            parsedNode = new BlockStatementNode(std::move(blockBody));
        } else if(type == astTypeCallExpression) {
            Node* callee = fn(getObjectFromMozJS(s_cx, obj, "callee"), currentBody, false);
            ArgumentVector arguments;

            JSObject* children = getObjectFromMozJS(s_cx, obj, "arguments");
            uint32_t siz = getArrayLengthFromMozJS(s_cx, children);
            for (uint32_t i = 0; i < siz; i++) {
                arguments.push_back(fn(getArrayElementFromMozJS(s_cx, children, i), currentBody, false));
            }
            parsedNode = new CallExpressionNode(callee, std::move(arguments));
        } else if(type == astTypeNewExpression) {
            Node* callee = fn(getObjectFromMozJS(s_cx, obj, "callee"), currentBody, false);
            ArgumentVector arguments;
            JSObject* children = getObjectFromMozJS(s_cx, obj, "arguments");
            uint32_t siz = getArrayLengthFromMozJS(s_cx, children);
            for (uint32_t i = 0; i < siz; i++) {
                arguments.push_back(fn(getArrayElementFromMozJS(s_cx, children, i), currentBody, false));
            }
            parsedNode = new NewExpressionNode(callee, std::move(arguments));
        } else if(type == astTypeObjectExpression) {
            PropertiesNodeVector propertiesVector;
            JSObject* children = getObjectFromMozJS(s_cx, obj, "properties");
            uint32_t siz = getArrayLengthFromMozJS(s_cx, children);
            for (uint32_t i = 0; i < siz; i++) {
                Node* n = fn(getArrayElementFromMozJS(s_cx, children, i), currentBody, false);
                ASSERT(n->type() == NodeType::Property);
                propertiesVector.push_back((PropertyNode *)n);
            }
            parsedNode = new ObjectExpressionNode(std::move(propertiesVector));
        } else if(type == astConditionalExpression) {
            parsedNode = new ConditionalExpressionNode(fn(getObjectFromMozJS(s_cx, obj, "test"), currentBody, false),
                    fn(getObjectFromMozJS(s_cx, obj, "consequent"), currentBody, false), fn(getObjectFromMozJS(s_cx, obj, "alternate"), currentBody, false));
        } else if(type == astTypeProperty) {
            PropertyNode::Kind kind = PropertyNode::Kind::Init;
            InternalString get(L"get");
            InternalString set(L"set");
            InternalString kinds(getStringFromMozJS(s_cx, obj, "kind"));
            if(get == kinds) {
                kind = PropertyNode::Kind::Get;
            } else if(set == kinds) {
                kind = PropertyNode::Kind::Set;
            }
            parsedNode = new PropertyNode(fn(getObjectFromMozJS(s_cx, obj, "key"), currentBody, false), fn(getObjectFromMozJS(s_cx, obj, "value"), currentBody, false), kind);
        } else if(type == astTypeMemberExpression) {
            parsedNode = new MemberExpressionNode(fn(getObjectFromMozJS(s_cx, obj, "object"), currentBody, false),
                    fn(getObjectFromMozJS(s_cx, obj, "property"), currentBody, false), getBooleanFromMozJS(s_cx, obj, "computed"));
        } else if(type == astTypeBinaryExpression) {
            parsedNode = new BinaryExpressionNode(fn(getObjectFromMozJS(s_cx, obj, "left"), currentBody, false),
                    fn(getObjectFromMozJS(s_cx, obj, "right"), currentBody, false), InternalString(getStringFromMozJS(s_cx, obj, "operator")));
        } else if(type == astTypeLogicalExpression) {
            parsedNode = new LogicalExpressionNode(fn(getObjectFromMozJS(s_cx, obj, "left"), currentBody, false),
                    fn(getObjectFromMozJS(s_cx, obj, "right"), currentBody, false), InternalString(getStringFromMozJS(s_cx, obj, "operator")));
        } else if(type == astTypeUpdateExpression) {
            parsedNode = new UpdateExpressionNode(fn(getObjectFromMozJS(s_cx, obj, "argument"), currentBody, false), InternalString(getStringFromMozJS(s_cx, obj, "operator")),
                    getBooleanFromMozJS(s_cx, obj, "prefix"));
        } else if(type == astTypeUnaryExpression) {
            parsedNode = new UnaryExpressionNode(fn(getObjectFromMozJS(s_cx, obj, "argument"), currentBody, false), InternalString(getStringFromMozJS(s_cx, obj, "operator")));
        } else if(type == astTypeIfStatement) {
            Node* a = NULL;
            if(getObjectFromMozJS(s_cx, obj, "alternate"))
                a = fn(getObjectFromMozJS(s_cx, obj, "alternate"), currentBody, false);
            parsedNode = new IfStatementNode(fn(getObjectFromMozJS(s_cx, obj, "test"),currentBody, false), fn(getObjectFromMozJS(s_cx, obj, "consequent"), currentBody, false), a);
        } else if(type == astTypeForStatement) {
            Node* init_node = NULL;
            if (getObjectFromMozJS(s_cx, obj, "init"))
                init_node = fn(getObjectFromMozJS(s_cx, obj, "init"), currentBody, false);
            parsedNode = new ForStatementNode(init_node, fn(getObjectFromMozJS(s_cx, obj, "test"), currentBody, false),
                    fn(getObjectFromMozJS(s_cx, obj, "update"), currentBody, false), fn(getObjectFromMozJS(s_cx, obj, "body"), currentBody, false));
        } else if(type == astTypeForInStatement) {
            JSObject* left = getObjectFromMozJS(s_cx, obj, "left");
            InternalString left_type(getStringFromMozJS(s_cx, left, "type"));
            Node* left_node = fn(left, currentBody, false);
            if (left_type == astTypeVariableDeclaration) {
                JSObject* left_children = getObjectFromMozJS(s_cx, left, "declarations");
                JSObject* zero = getArrayElementFromMozJS(s_cx, left_children, 0);
                left_node = fn(getObjectFromMozJS(s_cx, zero, "id"), currentBody, false);
            }
            parsedNode = new ForInStatementNode(left_node, fn(getObjectFromMozJS(s_cx, obj, "right"), currentBody, false),
                    fn(getObjectFromMozJS(s_cx, obj, "body"), currentBody, false), getBooleanFromMozJS(s_cx, obj, "each"));
        } else if(type == astTypeWhileStatement) {
            parsedNode = new WhileStatementNode(fn(getObjectFromMozJS(s_cx, obj, "test"), currentBody, false), fn(getObjectFromMozJS(s_cx, obj, "body"), currentBody, false));
        } else if(type == astTypeDoWhileStatement) {
            parsedNode = new DoWhileStatementNode(fn(getObjectFromMozJS(s_cx, obj, "test"), currentBody, false), fn(getObjectFromMozJS(s_cx, obj, "body"), currentBody, false));
        } else if(type == astTypeThisExpression) {
            parsedNode = new ThisExpressionNode();
        } else if(type == astTypeBreakStatement) {
            parsedNode = new BreakStatementNode();
        } else if(type == astTypeContinueStatement) {
            parsedNode = new ContinueStatementNode();
        } else if(type == astTypeReturnStatement) {
            Node* arg_node = NULL;
            if (getObjectFromMozJS(s_cx, obj, "argument")) {
                arg_node = fn(getObjectFromMozJS(s_cx, obj, "argument"), currentBody, false);
             }
            parsedNode = new ReturnStatmentNode(arg_node);
        } else if(type == astTypeEmptyStatement) {
            parsedNode = new EmptyStatementNode();
        } else if (type == astTypeTryStatement) {
           CatchClauseNodeVector guardedHandlers;
           JSObject* children = getObjectFromMozJS(s_cx, obj, "guardedHandlers");
           uint32_t siz = getArrayLengthFromMozJS(s_cx, children);
           for (uint32_t i = 0; i < siz; i++) {
                guardedHandlers.push_back(fn(getArrayElementFromMozJS(s_cx, children, i), currentBody, false));
           }

           Node* arg_node = NULL;
           if (!getObjectFromMozJS(s_cx, obj, "finalizer")) {
               parsedNode = new TryStatementNode(fn(getObjectFromMozJS(s_cx, obj, "block"), currentBody, false),
                       fn(getObjectFromMozJS(s_cx, obj, "handler"), currentBody, false), std::move(guardedHandlers), NULL);
           } else {
               parsedNode = new TryStatementNode(fn(getObjectFromMozJS(s_cx, obj, "block"), currentBody, false),
                       fn(getObjectFromMozJS(s_cx, obj, "handler"), currentBody, false), std::move(guardedHandlers), fn(getObjectFromMozJS(s_cx, obj, "finalizer"), currentBody, false));
           }
        } else if (type == astTypeCatchClause) {
           if (!getObjectFromMozJS(s_cx, obj, "guard")) {
                parsedNode = new CatchClauseNode(fn(getObjectFromMozJS(s_cx, obj, "param"), currentBody, false), NULL, fn(getObjectFromMozJS(s_cx, obj, "body"), currentBody, false));
            } else {
                parsedNode = new CatchClauseNode(fn(getObjectFromMozJS(s_cx, obj, "param"), currentBody, false), fn(getObjectFromMozJS(s_cx, obj, "guard"), currentBody, false), fn(getObjectFromMozJS(s_cx, obj, "body"), currentBody, false));
            }
        } else if (type == astTypeThrowStatement) {
            parsedNode = new ThrowStatementNode(fn(getObjectFromMozJS(s_cx, obj, "argument"), currentBody, false));
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
    Node* node = fn(JSVAL_TO_OBJECT(ret), &programBody, false);

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
            InternalString nonAtomicName = ((IdentifierNode *)currentNode)->nonAtomicName();
            auto iter = std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),name);
            if(name == strings->atomicArguments && iter == identifierInCurrentContext.end() && nearFunctionNode) {
                identifierInCurrentContext.push_back(strings->atomicArguments);
                nearFunctionNode->markNeedsArgumentsObject();
                iter = std::find(identifierInCurrentContext.begin(),identifierInCurrentContext.end(),name);
            }
            if(identifierInCurrentContext.end() == iter) {
                if(!instance->globalObject()->hasKey(nonAtomicName)) {
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
        } else if(type == NodeType::ConditionalExpression) {
            postAnalysisFunction(((ConditionalExpressionNode *)currentNode)->m_test, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((ConditionalExpressionNode *)currentNode)->m_consequente, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((ConditionalExpressionNode *)currentNode)->m_alternate, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::Property) {
            postAnalysisFunction(((PropertyNode *)currentNode)->m_key, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((PropertyNode *)currentNode)->m_value, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::MemberExpression) {
            postAnalysisFunction(((MemberExpressionNode *)currentNode)->m_object, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((MemberExpressionNode *)currentNode)->m_property, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::BinaryExpression) {
            postAnalysisFunction(((BinaryExpressionNode *)currentNode)->m_right, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((BinaryExpressionNode *)currentNode)->m_left, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::LogicalExpression) {
            postAnalysisFunction(((LogicalExpressionNode *)currentNode)->m_right, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((LogicalExpressionNode *)currentNode)->m_left, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::UpdateExpression) {
            postAnalysisFunction(((UpdateExpressionNode *)currentNode)->m_argument, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::UnaryExpression) {
            postAnalysisFunction(((UnaryExpressionNode *)currentNode)->m_argument, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::IfStatement) {
            postAnalysisFunction(((IfStatementNode *)currentNode)->m_test, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((IfStatementNode *)currentNode)->m_consequente, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((IfStatementNode *)currentNode)->m_alternate, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::ForStatement) {
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_init, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_body, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_test, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((ForStatementNode *)currentNode)->m_update, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::ForInStatement) {
            postAnalysisFunction(((ForInStatementNode *)currentNode)->m_left, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((ForInStatementNode *)currentNode)->m_right, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((ForInStatementNode *)currentNode)->m_body, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::WhileStatement) {
            postAnalysisFunction(((WhileStatementNode *)currentNode)->m_test, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((WhileStatementNode *)currentNode)->m_body, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::DoWhileStatement) {
            postAnalysisFunction(((DoWhileStatementNode *)currentNode)->m_test, identifierInCurrentContext, nearFunctionNode);
            postAnalysisFunction(((DoWhileStatementNode *)currentNode)->m_body, identifierInCurrentContext, nearFunctionNode);
        } else if(type == NodeType::ThisExpression) {

        } else if(type == NodeType::BreakStatement) {

        } else if(type == NodeType::ContinueStatement) {

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
