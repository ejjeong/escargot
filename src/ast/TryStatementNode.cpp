#include "Escargot.h"
#include "TryStatementNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/Environment.h"
#include "IdentifierNode.h"

namespace escargot {

TryStatementNode::TryStatementNode(Node *block, Node *handler, CatchClauseNodeVector&& guardedHandlers,  Node *finalizer)
            : StatementNode(NodeType::TryStatement)
    {
        m_block = (BlockStatementNode*) block;
        m_handler = (CatchClauseNode*) handler;
        m_guardedHandlers = guardedHandlers;
        m_finalizer = (BlockStatementNode*) finalizer;
    }

ESValue TryStatementNode::execute(ESVMInstance* instance)
{
    try {
        m_block->execute(instance);
    } catch(const ESValue& err) {
        instance->invalidateIdentifierCacheCheckCount();
        LexicalEnvironment* oldEnv = instance->currentExecutionContext()->environment();
        LexicalEnvironment* catchEnv = new LexicalEnvironment(new DeclarativeEnvironmentRecord(), oldEnv);
        instance->currentExecutionContext()->setEnvironment(catchEnv);
        instance->currentExecutionContext()->environment()->record()->setMutableBinding(m_handler->param()->name(),
                m_handler->param()->nonAtomicName()
                , err, false);
        m_handler->execute(instance);
    }
    return ESValue();
}

}
