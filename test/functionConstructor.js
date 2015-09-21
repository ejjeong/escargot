var a = Function("buffer", "print('hi');");
print(a.name);
a();
var b = Function("a", "print(a);");
print(b.name);

b(10);
b("str");
