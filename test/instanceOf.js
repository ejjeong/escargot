function foo() {
}
function foo1() {
}

var a = new foo();

print(a instanceof foo);
print(a instanceof foo1);
print(a instanceof Object);
print(foo instanceof Function);
