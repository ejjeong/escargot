function tta() {
    print('aaa');
}
var t = function tt() { print('hhhello'); }
t();

print(t.name);
print('---------');

var a = Function("print('hi');");
print(a);
print(a.name);
a();
var b = Function("aa", "print(aa);");
print(b.name);

b(10);
b("str");

var rrr = Function("return this;")();
function r() {
    return rrr;
}

