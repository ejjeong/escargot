var a;
a = 1;

/*
 * ProgramAST
 * [VariableDe...AST "a", AssignmentAST "a" 1]
 *
 * ESVMInstance->currentExecutionContext->currentEnv..()->record()->CreateMutableBinding("a");
 * ESVMInstance->currentExecutionContext->currentEnv..()->record()->SetMutableBinding("a",ESValue(1));
 */

