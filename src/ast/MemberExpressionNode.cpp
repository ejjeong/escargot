#include "Escargot.h"
#include "MemberExpressionNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"


namespace escargot {

ESValue* MemberExpressionNode::execute(ESVMInstance* instance)
{
    ESValue* obj = m_object->execute(instance)->ensureValue();
    //TODO string,number-> stringObject, numberObject;
    if(obj->isHeapObject() && obj->toHeapObject()->isJSObject()) {
        ESValue* property = m_property->execute(instance)->ensureValue();

        instance->currentExecutionContext()->setLastJSObjectMetInMemberExpressionNode(obj->toHeapObject()->toJSObject(),
                property);

        return obj->toHeapObject()->toJSObject()->find(property->toESString());
    } else {
        throw "TypeError";
    }
    return undefined;
}
}
