#include "Escargot.h"
#include "MemberExpressionNode.h"

#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"


namespace escargot {

ESValue* MemberExpressionNode::execute(ESVMInstance* instance)
{
    ESValue* obj = m_object->execute(instance)->ensureValue();
    //TODO string,number-> stringObject, numberObject;
    if(obj->isHeapObject() && obj->toHeapObject()->isJSObject()) {
        ESString propertyName;
        if(!m_computed && m_property->type() == NodeType::Identifier) {
            propertyName = ((IdentifierNode*)m_property)->name();
        } else {
            propertyName = m_property->execute(instance)->ensureValue()->toESString();
        }

        instance->currentExecutionContext()->setLastJSObjectMetInMemberExpressionNode(obj->toHeapObject()->toJSObject(),
                propertyName);

        JSObjectSlot* res = obj->toHeapObject()->toJSObject()->find(propertyName);
        if (res == NULL)
            return undefined;
        else
            return res;
    } else {
        throw "TypeError";
    }
    return undefined;
}
}
