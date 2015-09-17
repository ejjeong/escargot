#ifndef NewExpressionNode_h
#define NewExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class NewExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    NewExpressionNode(Node* callee, ArgumentVector&& arguments)
            : ExpressionNode(NodeType::NewExpression)
    {
        m_callee = callee;
        m_arguments = arguments;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESValue fn = m_callee->executeExpression(instance);
        if(!fn.isESPointer() || !fn.asESPointer()->isESFunctionObject())
            throw ESValue(TypeError::create(ESString::create(u"NewExpression: constructor is not an function object")));
        ESFunctionObject* function = fn.asESPointer()->asESFunctionObject();
        ESObject* receiver;
        if (function == instance->globalObject()->date()) {
            receiver = ESDateObject::create();
        } else if (function == instance->globalObject()->array()) {
            receiver = ESArrayObject::create(0);
        } else if (function == instance->globalObject()->string()) {
            receiver = ESStringObject::create();
        } else if (function == instance->globalObject()->regexp()) {
            receiver = ESRegExpObject::create(strings->emptyESString,ESRegExpObject::Option::None);
        } else if (function == instance->globalObject()->boolean()) {
            receiver = ESBooleanObject::create(ESValue(ESValue::ESFalseTag::ESFalse));
        } else if (function == instance->globalObject()->error()) {
            receiver = ESErrorObject::create();
        } else if (function == instance->globalObject()->referenceError()) {
            receiver = ReferenceError::create();
        } else if (function == instance->globalObject()->typeError()) {
            receiver = TypeError::create();
        } else if (function == instance->globalObject()->syntaxError()) {
            receiver = SyntaxError::create();
        } else if (function == instance->globalObject()->rangeError()) {
            receiver = RangeError::create();
        }
        // TypedArray
        else if (function == instance->globalObject()->int8Array()) {
            receiver = ESTypedArrayObject<Int8Adaptor>::create();
        } else if (function == instance->globalObject()->uint8Array()) {
            receiver = ESTypedArrayObject<Uint8Adaptor>::create();
        } else if (function == instance->globalObject()->int16Array()) {
            receiver = ESTypedArrayObject<Int16Adaptor>::create();
        } else if (function == instance->globalObject()->uint16Array()) {
            receiver = ESTypedArrayObject<Uint16Adaptor>::create();
        } else if (function == instance->globalObject()->int32Array()) {
            receiver = ESTypedArrayObject<Int32Adaptor>::create();
        } else if (function == instance->globalObject()->uint32Array()) {
            receiver = ESTypedArrayObject<Uint32Adaptor>::create();
        } else if (function == instance->globalObject()->uint8ClampedArray()) {
            receiver = ESTypedArrayObject<Uint8ClampedAdaptor>::create();
        } else if (function == instance->globalObject()->float32Array()) {
            receiver = ESTypedArrayObject<Float32Adaptor>::create();
        } else if (function == instance->globalObject()->float64Array()) {
            receiver = ESTypedArrayObject<Float64Adaptor>::create();
        } else if (function == instance->globalObject()->arrayBuffer()) {
            receiver = ESArrayBufferObject::create();
        } else {
            receiver = ESObject::create();
        }
        receiver->setConstructor(fn);
        if(function->protoType().isObject())
            receiver->set__proto__(function->protoType());
        else
            receiver->set__proto__(ESObject::create());

        ESValue* arguments = (ESValue*)alloca(sizeof(ESValue) * m_arguments.size());
        for(unsigned i = 0; i < m_arguments.size() ; i ++) {
            arguments[i] = m_arguments[i]->executeExpression(instance);
        }

        ESValue res = ESFunctionObject::call(instance, fn, receiver, arguments, m_arguments.size(), true);
        if (res.isObject())
            return res;
        return receiver;
    }

protected:
    Node* m_callee;
    ArgumentVector m_arguments;
};

}

#endif
