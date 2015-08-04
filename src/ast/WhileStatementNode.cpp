#include "Escargot.h"
#include "WhileStatementNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue WhileStatementNode::execute(ESVMInstance* instance)
{
    ESValue test = m_test->execute(instance);
    instance->currentExecutionContext()->setJumpPositionAndExecute([&](){
            jmpbuf_wrapper cont;
            int r = setjmp(cont.m_buffer);
            if (r != 1) {
                instance->currentExecutionContext()->pushContinuePosition(cont);
            } else {
                test = m_test->execute(instance);
            }
            while (test.toBoolean()) {
                m_body->execute(instance);
                test = m_test->execute(instance);
            }
            instance->currentExecutionContext()->popContinuePosition();
    });
    return ESValue();
}

}
