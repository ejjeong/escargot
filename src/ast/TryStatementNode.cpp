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

ESValue* TryStatementNode::execute(ESVMInstance* instance)
{
	try {
		m_block->execute(instance);
	} catch(ReferenceError& err) {
		instance->currentExecutionContext()->environment()->record()->createMutableBindingForAST(m_handler->param()->name(), false);
		instance->currentExecutionContext()->environment()->record()->setMutableBinding(m_handler->param()->name(), JSString::create(err.identifier()), false);
		m_handler->execute(instance);
		//instance->currentExecutionContext()->environment()->record()->deleteBinding(m_handler->param()->name());
	} catch(TypeError& err) {
		wprintf(L"TypeError\n");
	} catch(JSObject* err) {
		LexicalEnvironment* oldEnv = instance->currentExecutionContext()->environment();
		LexicalEnvironment* catchEnv = new LexicalEnvironment(new DeclarativeEnvironmentRecord(), oldEnv);
		instance->currentExecutionContext()->setEnvironment(catchEnv);
		instance->currentExecutionContext()->environment()->record()->setMutableBinding(m_handler->param()->name(), err, false);
		m_handler->execute(instance);
	}
	return esUndefined;
}

}
