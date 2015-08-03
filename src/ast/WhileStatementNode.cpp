#include "Escargot.h"
#include "WhileStatementNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue WhileStatementNode::execute(ESVMInstance* instance)
{
    ESValue test = m_test->execute(instance).ensureValue();
    instance->currentExecutionContext()->breakPosition([&](){
            while (test.toBoolean()) {
                m_body->execute(instance);
                test = m_test->execute(instance).ensureValue();
            }
            });
    return ESValue();
}

}
