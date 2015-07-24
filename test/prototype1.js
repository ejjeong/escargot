print("Function=");
print(Function);
print("Function.__proto__=");
print(Function.__proto__);
print("Function.prototype=");
print(Function.prototype);

print("Object=");
print(Object);
print("Object.__proto__=");
print(Object.__proto__);
print("Object.prototype=");
print(Object.prototype);

function foo()
{
}
print("foo=");
print(foo);
print("foo.name=");
print(foo.name);
print("foo.__proto__=");
print(foo.__proto__);
print("foo.prototype=");
print(foo.prototype);

/*var fe = function infe() {};
print("fe=");
print(fe);
print("fe.name=");
print(fe.name);
print("fe.__proto__=");
print(fe.__proto__);
print("fe.prototype=");
print(fe.prototype);*/


foo.prototype.foo = function() {
   print("call foo.foo");
};

var instanceOfFoo = new foo;
print(instanceOfFoo.constructor);
print(instanceOfFoo.__proto__);
print(instanceOfFoo.__proto__.constructor);
instanceOfFoo.foo();
function foo2()
{
}

foo2.prototype.__proto__ = foo.prototype;
foo2.prototype.foo2 = function() { print("asdf") };

var insfoo = new foo2;
insfoo.foo();
insfoo.foo2();
