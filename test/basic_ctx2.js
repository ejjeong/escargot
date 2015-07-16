var b = 1;
function foo() {
  var a;
  a = 1;
  b = 2;
  print(a);
  print(b);
}
foo();

/*
 * ProgramAST
 [VariableDe...AST "b", VariableDe...AST "foo"<
 AssignmentAST "b" 1, AssignmentAST "foo", FunctionExpressonAST[
 VDAST "a"
 ASSGNMENTAST "a" 1
 ASSGNMENTAST "b" 2
 ]
 CallAST "foo", []]

 ESVMInstance->currentExecutionContext->currentEnv..()->record()->CreateMutableBinding("b");
 ESVMInstance->currentExecutionContext->currentEnv..()->record()->CreateMutableBinding("foo");
 ESVMInstance->currentExecutionContext->currentEnv..()->record()->SetMutableBinding("b",ESValue(1));
 ESVMInstance->currentExecutionContext->currentEnv..()->record()->SetMutableBinding("foo",JSFunction(
 ESVMInstance->currentExecutionContext->currentEnv..(), this-><....> ));

 ESValue fn = ESVMInstance->currentExecutionContext->currentEnv..()->record()->GetBindingValue("foo");
 ESFunctionCaller::call(fn, <global>, NULL, 0, ESVMInstance);
 */
