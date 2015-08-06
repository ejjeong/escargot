#include "Escargot.h"
#include "ForInStatementNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "ast/AssignmentExpressionNode.h"

namespace escargot {

ESValue ForInStatementNode::execute(ESVMInstance* instance)
{
    ESValue exprValue = m_right->execute(instance);
    if (exprValue.isNull() || exprValue.isUndefined())
        return ESValue();

    ESObject* obj = exprValue.toObject();
    std::vector<InternalString> propertyNames;
    obj->enumeration([&propertyNames](const InternalString& key, ESSlot* slot) {
        propertyNames.push_back(key);
    });
    instance->currentExecutionContext()->setJumpPositionAndExecute([&](){
        jmpbuf_wrapper cont;
        int r = setjmp(cont.m_buffer);
        if (r != 1) {
            instance->currentExecutionContext()->pushContinuePosition(cont);
        }
        for (unsigned int i=0; i<propertyNames.size(); i++) {
            if (obj->hasKey(propertyNames[i])) {
                ESString* name = ESString::create(propertyNames[i]);
                AssignmentExpressionNode::writeValue(instance, m_left, name);
                m_body->execute(instance);
            }
        }
        instance->currentExecutionContext()->popContinuePosition();
    });
    return ESValue();
}

}
