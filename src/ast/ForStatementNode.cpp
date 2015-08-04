#include "Escargot.h"
#include "ForStatementNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue ForStatementNode::execute(ESVMInstance* instance)
{
    if (m_init)
        m_init->execute(instance);
    ESValue test = m_test->execute(instance);
    instance->currentExecutionContext()->breakPosition([&](){
        while (test.toBoolean()) {
            m_body->execute(instance);
            m_update->execute(instance);
            test = m_test->execute(instance);
        }

    });
    return ESValue();
}

}
